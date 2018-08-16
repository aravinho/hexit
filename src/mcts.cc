#include "mcts.h"

#include <iostream>

#include <unistd.h>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <map>
#include <sys/types.h>
#include <signal.h>
#include <fstream>
#include <stdexcept>
#include <stdlib.h>
#include <random>
#include <dirent.h>
#include <sstream>






using namespace std;




bool print_enabled = true;

void print(string message) {
	if (print_enabled) {
		cout << message << endl;
	}
}




StateVector::StateVector() {

}


StateVector::StateVector(const vector<double>& vec) {
	for (double d : vec) {
		this->_sv.push_back(d);
	}
}

double StateVector::at(int k) const {
	ASSERT(k >= 0 && k < this->_sv.size(), "Index " << k << " is out of range in StateVector::at");
	return this->_sv[k];
}

StateVector::~StateVector() {
}

string StateVector::asCSVString() const {
	
	string s = "";

	for (int pos = 0; pos < this->_sv.size(); pos++) {
		s += to_string(this->_sv[pos]);
		if (pos != this->_sv.size() - 1) {
			s += ",";
		}
	}

	return s;
}



ActionDistribution::ActionDistribution() {
	this->action_dist = new vector<double>(9, 0.0);
}

ActionDistribution::ActionDistribution(vector<double>* action_dist) {
	this->action_dist = action_dist;
}

ActionDistribution::ActionDistribution(int num_actions, string csv_line, char delimiter) {
	vector<double>* action_dist = new vector<double>(num_actions);
	string token;
	istringstream token_stream(csv_line);
	while (getline(token_stream, token, delimiter)) {
		double d = stod(token);
		action_dist->push_back(d);
	}
	this->action_dist = action_dist;
}	

ActionDistribution::~ActionDistribution() {
	if (this->action_dist != NULL) {
		delete this->action_dist;
	}
}
vector<double>* ActionDistribution::asVector() {
	return this->action_dist;
}

double ActionDistribution::at(int k) const {
	ASSERT(0 <= k && k < this->action_dist->size(), "Cannot get the " << k << "th element of this ActionDistribution");
	return this->action_dist->at(k);
}






MCTS_Node::MCTS_Node(EnvState* state, bool is_root, int num_simulations, bool sample_actions, bool requires_nn, bool use_rave,
	double c_b, double c_rave, double w_a) {
	
	this->is_root = is_root;
	this->state = state;

	this->total_num_simulations = num_simulations; // irrelevant if not a root
	this->num_simulations_finished = 0; // irrelevant if not a root
	
	this->depth = 0; // if this is a child node created by makeChild, depth will be overriden by a call to setDepth from makeChild
	this->parent = NULL; // if this is a child node created by makeChild, parent will be set by a call to setParent from makeChild

	// store a pointer to the root of the tree 
	// if this is a child node created by makeChild, root will be set by a call to setRoot from makeChild
	if (is_root) {
		this->root = this;
	} else {
		this->root = NULL;
	}

	// if this node is the K-th child of its parent, then its child_index is K.
	// if this is a child node created by makeChild, its child_index will be set by a call to setChildIndex from makeChild
	this->child_index = -1;

	this->num_actions = state->numActions();
	this->children = new vector<MCTS_Node*>(this->num_actions, NULL); // initialize all children to NULL

	// these flags are used to determine whether a node has submitted its state vector to an NN apprentice yet
	// (and whether it has yet processed the results)
	this->submitted_to_nn = false;
	this->received_nn_results = false;

	// these stats are used to calculate the best action to take during MCTS.
	this->num_node_visits = 0; this->num_node_visits_rave = 0;
	this->num_edge_traversals = new vector<int>(this->num_actions, 0);
	this->num_edge_traversals_rave = new vector<int>(this->num_actions, 0);
	this->edge_rewards = new vector<double>(this->num_actions, 0.0);
	this->edge_rewards_rave = new vector<double>(this->num_actions, 0.0);

	// this is the state vector that is sent to the neural net.
	vector<double>* sv = new vector<double>();
	state->makeStateVector(sv);
	this->state_vector = new StateVector(*sv);
	// initialize the AD to NULL
	this->nn_result = NULL;

	// whether to sample actions proportional to scores (if false, uses argmax)
	this->sample_actions = sample_actions;
	// whether this node uses an NN apprentice during MCTS
	this->requires_nn = requires_nn;
	// whether to use RAVE when calculating action scores
	this->use_rave = use_rave;

	// hyperparameters
	this->c_b = c_b;
	ASSERT(c_rave > 0, "Must have a positive c_rave");
	this->c_rave = c_rave;
	this->w_a = w_a;

}


MCTS_Node::~MCTS_Node() {
	
	
	// do not delete this->state (or this->state_vector) because we don't want the state vector to get deleted
	// likewise, do not delete this->nn_result
	// deleting these will cause the ThreadManager's queues to be messed up.

	// delete state
	delete this->state;

	// try deleting state vector and action distribution
	if (this->state_vector != NULL) {
		delete this->state_vector;
	} 
	if (this->nn_result != NULL) {
		delete this->nn_result;
	}


	
	// delete all N and R vectors
	if (this->num_edge_traversals != NULL) {
		delete this->num_edge_traversals; // n(s,a)
	}
	
	if (this->edge_rewards != NULL) {
		delete this->edge_rewards; // r(s, a)
	}

	if (this->num_edge_traversals_rave != NULL) {
		delete this->num_edge_traversals_rave; // n(s,a)-rave
	}

	if (this->edge_rewards_rave != NULL) {
		delete this->edge_rewards_rave; // r(s,a)-rave
	}

	// delete each child recursively
	for (MCTS_Node* child : *(this->children)) {
		if (child != NULL) {
			delete child;
		}
	}
	// delete the children vector itself
	delete this->children;

}


void MCTS_Node::deleteTree() {
	ASSERT(this->isRoot(), "Cannot delete a tree starting from a non-root node");

	for (MCTS_Node* child : *(this->children)) {
		if (child != NULL) {
			delete child;
		}
	}
	
}






bool MCTS_Node::isTerminal() const {
	ASSERT(this->state != NULL, "This->state is NULL in MCTS_Node::isTerminal");
	return this->state->isTerminalState();
}

bool MCTS_Node::isRoot() const {
	return this->is_root;
}

int MCTS_Node::numSimulationsFinished() const {
	return this->num_simulations_finished;
}

bool MCTS_Node::simulationsFinished() const {
	return this->num_simulations_finished >= this->total_num_simulations;
}

void MCTS_Node::markSimulationFinished() {
	this->num_simulations_finished += 1;
}




bool MCTS_Node::usesRave() const {
	return this->use_rave;
}

bool MCTS_Node::requiresNN() const {
	return this->requires_nn;
}

bool MCTS_Node::neverSubmittedToNN() const{
	return !this->submitted_to_nn;
}

void MCTS_Node::markSubmittedToNN() {
	this->submitted_to_nn = true;
}

bool MCTS_Node::awaitingNNResults() const {
	return !this->received_nn_results;
}

void MCTS_Node::markReceivedNNResults() {
	this->received_nn_results = true;
}





int MCTS_Node::getNumActions() const {
	return this->num_actions;
}

int MCTS_Node::getDepth() const {
	return this->depth;
}

MCTS_Node* MCTS_Node::setDepth(int depth) {
	ASSERT(depth >= 0, "Cannot set a negative depth");
	this->depth = depth;
	return this;
}

MCTS_Node* MCTS_Node::getParent() const {
	return this->parent; // may be null if this is a root
}

MCTS_Node* MCTS_Node::setParent(MCTS_Node* parent) {
	ASSERT(parent != NULL, "Cannot set a null parent");
	this->parent = parent;
	return this;
}

MCTS_Node* MCTS_Node::getChild(int k) const {
	ASSERT(0 <= k && k < this->num_actions, "Cannot get the " << k << "th child");
	return this->children->at(k); // may be null
}

MCTS_Node* MCTS_Node::setChild(MCTS_Node* child, int k) {
	ASSERT(child != NULL, "Cannot set the " << k << "th child to be null");
	ASSERT(0 <= k && k < this->num_actions, "Cannot set the " << k << "th child");
	this->children->at(k) = child;
	return this;
}

int MCTS_Node::getChildIndex() const {
	return child_index; // -1 if root
}

MCTS_Node* MCTS_Node::setChildIndex(int k) {
	ASSERT(0 <= k && k < this->num_actions, "Cannot set the child index of a node to be " << k);
	this->child_index = k;
	return this;
}



MCTS_Node* MCTS_Node::getRoot() const {
	return this->root; // just this if this is a root
}

MCTS_Node* MCTS_Node::setRoot(MCTS_Node* root) {
	ASSERT(root != NULL, "Cannot set a null root");
	this->root = root;
	return this;
}

MCTS_Node* MCTS_Node::makeChild(int k) {

	// terminal nodes cannot have children
	if (this->isTerminal()) {
		return this;
	}
	// if this node has already made its k-th child, don't make a new one.
	if (this->getChild(k) != NULL) {
		return this->getChild(k);
	}

	// create the child node
	MCTS_Node* child_node =
		new MCTS_Node(this->state->nextState(k), false /* is_root */, this->total_num_simulations, this->sample_actions, this->requires_nn, this->use_rave);
	
	// set its parent, child index, depth and root
	child_node->setParent(this);
	child_node->setChildIndex(k);
	child_node->setDepth(this->depth + 1);
	child_node->setRoot(this->root);

	// set the new node as the K-th child of this node
	this->setChild(child_node, k);

	return child_node;
}





EnvState* MCTS_Node::getState() const {
	return this->state;
}

StateVector* MCTS_Node::getStateVector() const {
	return this->state_vector;
}

void MCTS_Node::setNNActionDistribution(ActionDistribution* ad) {
	ASSERT(ad != NULL, "Cannot set a null action distribution");
	this->nn_result = ad;
}

vector<int>* MCTS_Node::getActionCounts() const {
	return this->num_edge_traversals;
}
	
void MCTS_Node::getActionDistribution(vector<double>* action_dist) const {
	ASSERT(action_dist != NULL, "Cannot populate a null action_dist vector");
	ASSERT(action_dist->size() >= this->num_actions, "There must be at least " << this->num_actions << " elements in action_dist");
	
	if (this->num_node_visits == 0) {
		return;
	}

	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		action_dist->at(action_num) = ((double) this->num_edge_traversals->at(action_num)) / ((double) this->num_node_visits);
	}
}







int MCTS_Node::N() const {
	return this->num_node_visits;
}


int MCTS_Node::N(int a) const {
	ASSERT(0 <= a && a < this->num_actions, "Cannot get num_edge_traversals for the " << a << "th child");
	return this->num_edge_traversals->at(a);
}

double MCTS_Node::R(int a) const {
	ASSERT(0 <= a && a < this->num_actions, "Cannot get edge_rewards for the " << a << "th child");
	return this->edge_rewards->at(a);
}

int MCTS_Node::NRave() const {
	return this->num_node_visits_rave;
}


int MCTS_Node::NRave(int a) const {
	ASSERT(0 <= a && a < this->num_actions, "Cannot get num_edge_traversals for the " << a << "th child");
	return this->num_edge_traversals_rave->at(a);
}

double MCTS_Node::RRave(int a) const {
	ASSERT(0 <= a && a < this->num_actions, "Cannot get edge_rewards for the " << a << "th child");
	return this->edge_rewards_rave->at(a);
}


vector<int>* MCTS_Node::edgeTraversals() const {
	return this->num_edge_traversals;
}

vector<double>* MCTS_Node::edgeRewards() const {
	return this->edge_rewards;
}

vector<int>* MCTS_Node::edgeTraversalsRave() const {
	return this->num_edge_traversals_rave;
}

vector<double>* MCTS_Node::edgeRewardsRave() const {
	return this->edge_rewards_rave;
}



void MCTS_Node::getMeanRewards(vector<double>* mean_reward_vec) const {
	ASSERT(mean_reward_vec != NULL, "Cannot have a null mean reward vec");
	ASSERT(mean_reward_vec->size() >= this->num_actions, "mean_reward_vec must have size at least " << this->num_actions);
	for (int action_num = 0; action_num < this->num_actions; action_num++) {

		double total_reward = this->edge_rewards->at(action_num);
		int num_taken = this->num_edge_traversals->at(action_num);

		if (num_taken == 0) {
			mean_reward_vec->at(action_num) = 0.0;
		} else {
			mean_reward_vec->at(action_num) = total_reward / num_taken;
		}
		
	}
}



void MCTS_Node::printMeanReward() const {
	int num_actions = this->getState()->numActions();
	vector<double> mean_rewards(num_actions, 0.0);
	for (int action_num = 0; action_num < num_actions; action_num++) {
		int n; double r;
		if (this->use_rave) {
			n = this->num_edge_traversals_rave->at(action_num);
			r = this->edge_rewards_rave->at(action_num);
		} else {
			n = this->num_edge_traversals->at(action_num);
			r = this->edge_rewards->at(action_num);
		}
		if (r != 0) {
			mean_rewards[action_num] = r / (double (n));
		}
	}

	int dim = (int) sqrt(num_actions);


	printVector(mean_rewards, "Mean rewards:", dim);
}

void MCTS_Node::printExplorationTerm() const {
	int num_actions = this->getState()->numActions();
	vector<double> exp_terms(num_actions, 0.0);
	if (N() == 0) {

	}
	double numer = N();
	for (int action_num = 0; action_num < num_actions; action_num++) {
		int n;
		if (this->use_rave) {
			n = this->num_edge_traversals_rave->at(action_num);
		} else {
			n = this->num_edge_traversals->at(action_num);
		}
		if (n != 0) {
			exp_terms[action_num] = this->c_b * sqrt(log(numer / ((double) n)));
		}
	}

	int dim = (int) sqrt(num_actions);
	printVector(exp_terms, "Exploration Exploitation Term:", dim);
}




void MCTS_Node::computeUCT(vector<double>* action_scores) const {

	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores must be at least " << this->num_actions);
	
	// if never been to this node, return all 0s (uniform)
	if (N() == 0) {
		return;
	}

	double avg_reward_term;
	double avg_log_visits_term;
	double score;

	// for each action, compute the score
	for (int a = 0; a < this->num_actions; a++) {

		if (N(a) == 0) {
			avg_reward_term = 0;
		} else {
			int turn = this->state->turn();
			avg_reward_term = (R(a) * turn) / N(a);
			// make sure we "minimize reward" (maximize negative reward if it is player 2 (player -1)'s turn)
		} 

		avg_log_visits_term = log(N()) / (N(a) + 1);
		score = avg_reward_term + (this->c_b * sqrt(avg_log_visits_term));
		action_scores->at(a) = score;

	}

}

void MCTS_Node::computeUCT_NN(vector<double>* action_scores) const {

	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores must be at least " << this->num_actions);
	ASSERT(this->received_nn_results, "Must have received NN results before computing UCT-NN");

	// if never been to this node, return all 0s (uniform)
	if (N() == 0) {
		return;
	}

	// compute UCT scores
	vector<double>* uct_scores = new vector<double>(this->num_actions, 0.0);
	this->computeUCT(uct_scores);

	// compute the NN term and add it to the UCT term
	double nn_term_numerator, nn_term_denominator, nn_term;
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		nn_term_numerator = this->nn_result->at(action_num);
		nn_term_denominator = ((double) this->N(action_num) + 1);
		ASSERT(nn_term_denominator > 0, "nn_term denominator must be positive");
		nn_term = this->w_a * (nn_term_numerator / nn_term_denominator);
		action_scores->at(action_num) = uct_scores->at(action_num) + nn_term;
	}
}

void MCTS_Node::computeUCT_Rave(vector<double>* action_scores) const {

	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores must be at least " << this->num_actions);
	
	// if never been to this node, return all 0s (uniform)
	if (NRave() == 0) {
		return;
	}

	double avg_reward_term;
	double avg_log_visits_term;
	double score;

	// for each action, compute the score
	for (int a = 0; a < this->num_actions; a++) {

		if (NRave(a) == 0) {
			avg_reward_term = 0;
		} else {
			int turn = this->state->turn();
			avg_reward_term = (RRave(a) * turn)/NRave(a); // make sure we "minimize reward" (maximize negative reward if it is player 2 (player -1)'s turn)
		} 

		avg_log_visits_term = log(NRave()) / (NRave(a) + 1);
		score = avg_reward_term + (this->c_b * sqrt(avg_log_visits_term));
		action_scores->at(a) = score;

	}

}

void MCTS_Node::computeUCT_U_Rave(vector<double>* action_scores) const {


	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores must be at least " << this->num_actions);

	// if never been to this node, return all 0s (uniform)
	if (N() == 0) {
		return;
	}

	// compute UCT and UCT_Rave scores
	vector<double>* uct_scores = new vector<double>(this->num_actions, 0.0);
	vector<double>* uct_rave_scores = new vector<double>(this->num_actions, 0.0);
	this->computeUCT(uct_scores);
	this->computeUCT_Rave(uct_scores);

	// compute beta, which controls the weight given to the RAVE estimate
	double beta_numerator = (double) this->c_rave;
	double beta_denominator = ((double) 3*this->N() + this->c_rave);
	ASSERT(beta_denominator > 0, "Beta denominator must be positive");
	double beta = sqrt(beta_numerator / beta_denominator);

	// compute the weighted average of UCT and UCT_Rave.
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		action_scores->at(action_num) = (beta * uct_rave_scores->at(action_num)) + ((1 - beta) * uct_scores->at(action_num));
	}

	// delete vectors to free up space
	delete uct_scores;
	delete uct_rave_scores;

}

void MCTS_Node::computeUCT_NN_Rave(vector<double>* action_scores) const {

	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores must be at least " << this->num_actions);
	ASSERT(this->received_nn_results, "Must have received NN results before computing UCT-NN");

	// if never been to this node, return all 0s (uniform)
	if (N() == 0) {
		return;
	}

	// compute UCT scores
	vector<double>* uct_scores = new vector<double>(this->num_actions, 0.0);
	this->computeUCT_U_Rave(uct_scores);

	// compute the NN term and add it to the UCT term
	double nn_term_numerator, nn_term_denominator, nn_term;
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		nn_term_numerator = this->nn_result->at(action_num);
		nn_term_denominator = ((double) this->N(action_num) + 1);
		ASSERT(nn_term_denominator > 0, "nn_term denominator must be positive");
		nn_term = this->w_a * (nn_term_numerator / nn_term_denominator);
		action_scores->at(action_num) = uct_scores->at(action_num) + nn_term;
	}

}


void MCTS_Node::computeActionScores(vector<double>* action_scores) const {

	ASSERT(action_scores != NULL, "Cannot pass a null action_scores vector to computeActionScores");
	ASSERT(action_scores->size() >= this->num_actions, "The size of action_scores (" << action_scores->size() << ") must be at least " << this->num_actions);

	// vanilla UCT
	if (!this->requires_nn && !this->use_rave) {
		this->computeUCT(action_scores);
	}

	// UCT-NN
	else if (this->requires_nn && !this->use_rave) {
		this->computeUCT_NN(action_scores);
	}

	// UCT-U-RAVE
	else if (!this->requires_nn && this->use_rave) {
		this->computeUCT_U_Rave(action_scores);
	}

	else if (this->requires_nn && this->use_rave) {
		this->computeUCT_NN_Rave(action_scores);
	}

	else {
		ASSERT(false, "Some incorrect combination of requires_nn and use_rave");
	}
	
}








int MCTS_Node::sampleLegalAction(const vector<double>& action_scores) {

	ASSERT(action_scores.size() >= this->num_actions, "Action dist must contain a score for each of the " << this->num_actions << " actions in sampleLegalAction");

	// compute mask
	vector<bool> mask(this->num_actions, false);
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		mask[action_num] = this->state->isLegalAction(action_num);
	}

	// compute the softmax of each score
	vector<double>* softmaxes = new vector<double>(this->num_actions, 0.0);
	computeSoftmaxWithMask(action_scores, mask, softmaxes);

	// return an action, sampled proportional to the softmax of its score
	int sampled_action = sampleProportionalToWeights(*softmaxes);
	delete softmaxes;
	return sampled_action;

    
}


int MCTS_Node::bestLegalAction(const vector<double>& action_scores) {

	ASSERT(action_scores.size() >= this->num_actions, "Action dist must contain a score for each of the " << this->num_actions << " actions in bestLegalAction");

	// compute mask
	vector<bool> mask(this->num_actions, false);
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		mask[action_num] = this->state->isLegalAction(action_num);
	}

	int best_action = argmaxWithMask(action_scores, mask);
	return best_action;

}



MCTS_Node* MCTS_Node::chooseBestAction() {
	profiler.start("chooseBestAction");

	// there are no actions to take in terminal states
	if (this->isTerminal()) {
		profiler.stop("chooseBestAction");
		return this;
	}
	// compute scores for each action (UCT, UCT-NN, UCT-RAVE, UCT-NN-RAVE, etc)
	vector<double>* action_scores = new vector<double>(this->num_actions, 0.0);
	this->computeActionScores(action_scores);

	// either sample an action with softmax probabilities, or take the argmax of the action scores
	int chosen_action;
	if (this->sample_actions) {
		chosen_action = sampleLegalAction(*action_scores);
	} else {
		chosen_action = bestLegalAction(*action_scores);
	}
	
	// create (or grab if already created) the child node obtained by taking the chosen_action
	MCTS_Node* child_node = this->makeChild(chosen_action);
	profiler.stop("chooseBestAction");
	return child_node;
}



inline void MCTS_Node::updateStats(int chosen_action, double reward, bool update_rave_stats) {
	if (update_rave_stats) {
		profiler.start("updateStats from rave");
	} else {
		profiler.start("updateStats");
	}
	ASSERT(0 <= chosen_action && chosen_action < this->num_actions, "Cannot update stats for action " << chosen_action);
	
	if (!update_rave_stats) {
		this->num_node_visits += 1;
		this->num_edge_traversals->at(chosen_action) += 1;
		this->edge_rewards->at(chosen_action) += reward;
	} else {
		this->num_node_visits_rave += 1;
		this->num_edge_traversals_rave->at(chosen_action) += 1;
		this->edge_rewards_rave->at(chosen_action) += reward;
	}
	if (update_rave_stats) {
		profiler.stop("updateStats from rave");
	} else {
		profiler.stop("updateStats");
	}

}


void MCTS_Node::updateStatsRave(const vector<int>& chosen_actions, double reward) {
	profiler.start("updateStatsRave");
	ASSERT(chosen_actions.size() < this->num_actions, "The chosen_actions vector should not have more than " << this->num_actions << " elements");
	for (int chosen_action : chosen_actions) {

		this->updateStats(chosen_action, reward, true /* update_rave_stats */);
	}
	profiler.stop("updateStatsRave");
}












MCTS_Node* propagateStats(MCTS_Node* node) {

	profiler.start("propagateStats");

	ASSERT(node != NULL, "Cannot propagate stats starting at a null node");
	// the given node is terminal, start here and propagate up
	double reward = ((double) node->getState()->reward());
	double max_reward = node->getState()->maxReward();
	reward /= max_reward;

	// track all the actions made in this simulation, to facilitate RAVE stats updates
	// these actions will be stored in reverse order, with the final action being the zeroth element
	vector<int> player1_actions;
	vector<int> player2_actions;
	
	MCTS_Node* curr_node = node;
	int chosen_action = curr_node->getChildIndex();

	while (curr_node != NULL) {

		// there are no stats for the terminal state
		if (curr_node->isTerminal()) {
			curr_node = curr_node->getParent();
			continue;
		}

		// whether or not this tree uses RAVE, update the normal stats N(s), N(s, a) and R(s,a) for this node
		curr_node->updateStats(chosen_action, reward);
		
		// if this tree uses RAVE, update additional stats using the all-moves-as-first method
		if (curr_node->usesRave()) {
			// determine which player's turn it is on this move
			// add this action to the list of actions that this player has taken in this simulation
			// update this node's stats using the all-moves-as-first method
			int turn = curr_node->getState()->turn();
			ASSERT(turn == 1 || turn == -1, "Turn must have been 1 or -1");
			if (turn == 1) {
				player1_actions.push_back(chosen_action);
				curr_node->updateStatsRave(player1_actions, reward);
			} else {
				player2_actions.push_back(chosen_action);
				curr_node->updateStatsRave(player2_actions, reward);
			}

		}
		
		// if we've reached the top of the tree (a root node), mark this simulation as finished 
		if (curr_node->isRoot()) {
			curr_node->markSimulationFinished();
			profiler.stop("propagateStats");
			return curr_node;
		}

		// update variables to prepare for next iteration up the tree
		chosen_action = curr_node->getChildIndex();
		curr_node = curr_node->getParent();

	}
	
	ASSERT(false, "propagateStats should have returned before this point");

}


MCTS_Node* rolloutSimulation(MCTS_Node* node) {
	profiler.start("rolloutSimulation");

	ASSERT(node != NULL, "Cannot roll out simulation starting at a null node");

	// choose a random action, take it. repeat until terminal action, return terminal action
	MCTS_Node* curr_node = node;
	while (!curr_node->isTerminal()) {
		int random_action = curr_node->getState()->randomAction();
		curr_node = curr_node->makeChild(random_action);
	}

	profiler.stop("rolloutSimulation");
	return curr_node;
}





MCTS_Node* runMCTS(MCTS_Node* node, int max_depth, ActionDistribution* ad) {

	ASSERT(node != NULL, "Cannot have a null node in runMCTS");
	ASSERT(max_depth > 0, "Must have positive max depth");

	profiler.start("runMCTS");

	MCTS_Node* curr_node = node;

	while (true) {

		ASSERT(curr_node != NULL, "curr_node is NULL");


		// if we are at the root and have finished all the simulations, break
		if (curr_node->isRoot() && curr_node->simulationsFinished()) {
			curr_node->deleteTree();
			break;
		}

		// if terminal state, do stats propagation back up to the root
		if (curr_node->isTerminal()) {
			curr_node = propagateStats(curr_node); // should return root node
			continue;
		}

		// if we are at max depth, perform rollout
		if (curr_node->getDepth() == max_depth) {
			curr_node = rolloutSimulation(curr_node); // should return terminal node
			continue;
		}

		// if at an intermediate node, look to see if the NN action distribution is already fetched
		// if never submitted, submit self and wait.  
		if (curr_node->requiresNN()) {
			if (curr_node->neverSubmittedToNN()) {
				curr_node->markSubmittedToNN();
				profiler.stop("runMCTS");
				return curr_node;
			}
			// if was previously waiting for AD, grab the given one and set it as a field
			if (curr_node->awaitingNNResults()) {
				ASSERT(ad != NULL, "Cannot set a null action distribution");
				curr_node->setNNActionDistribution(ad);
				curr_node->markReceivedNNResults();
			}
		}


		// get the best child and iterate

		curr_node = curr_node->chooseBestAction();

	}

	profiler.stop("runMCTS");
	return curr_node;
}


MCTS_Node* runAllSimulations(MCTS_Node* node, int max_depth) {

	ASSERT(node != NULL, "Cannot run all simulations with a null node");
	ASSERT(max_depth > 0, "Must have positive max depth");

	while (!node->simulationsFinished()) {
		node = runMCTS(node, max_depth, NULL /* ad */);
	}
	return node;
}












int writeStatesToFile(vector<MCTS_Node*>* nodes, string dirname, int next_file_num, int states_per_file) {

  	string x_filename;
	ofstream x_file;
	MCTS_Node* node;
	int file_num;

	int node_num;
	for (node_num = 0; node_num < nodes->size(); node_num++) {

		// open a new file if necessary
		if (node_num % states_per_file == 0) {
			file_num = next_file_num + (node_num / states_per_file);
			x_filename = dirname + to_string(file_num) + ".csv";
			x_file = ofstream(x_filename);
		}

		// make sure file was opened properly
		ASSERT(x_file.is_open(), "Unable to open file " << x_filename);

		// write the node's state
		node = nodes->at(node_num);
		x_file << node->getState()->asCSVString() << "\n";

		// close the current file if necessary
		if ((node_num + 1) % states_per_file == 0) {
			x_file.close();
		}
	}

	// close the last file if needed
	if (x_file.is_open()) {
		x_file.close();
	}

	return node_num;
   
}



int writeActionDistributionsToFile(vector<MCTS_Node*>* nodes, string dirname, int next_file_num, int states_per_file) {

  	string y_filename;
	ofstream y_file;
	MCTS_Node* node;
	int file_num;

	int node_num;
	for (node_num = 0; node_num < nodes->size(); node_num++) {

		// open a new file if necessary
		if (node_num % states_per_file == 0) {
			file_num = next_file_num + (node_num / states_per_file);
			y_filename = dirname + to_string(file_num) + ".csv";
			y_file = ofstream(y_filename);
		}

		// make sure file was opened properly
		ASSERT(y_file.is_open(), "Unable to open file " << y_filename);

		// write the node's action distribution
		node = nodes->at(node_num);
		vector<double>* action_dist = new vector<double>(node->getNumActions(), 0.0);
		node->getActionDistribution(action_dist);


		y_file << asCSVString(*action_dist) << "\n";

		// close the current file if necessary
		if ((node_num + 1) % states_per_file == 0) {
			y_file.close();
		}
	}

	// close the last file if needed
	if (y_file.is_open()) {
		y_file.close();
	}

	return node_num;

   
}


int writeBatchToFile(vector<MCTS_Node*>* nodes, string base_dirname, int states_per_file) {

	pair<string, string> subdirs = prepareDirectories(base_dirname);
	string x_dirname = subdirs.first, y_dirname = subdirs.second;

	// if there are already data files in this directory, make sure we do not override them
	int next_x_file_num = nextAvailableFileNum(x_dirname);
	int next_y_file_num = nextAvailableFileNum(y_dirname);
	ASSERT(next_x_file_num >= 0, "next_x_file_num " << next_x_file_num << " must be nonnegative");
	ASSERT(next_x_file_num == next_y_file_num, "next_x_file_num " << next_x_file_num << " must equal next_y_file_num " << next_y_file_num);

	// write the states and action distributions to files, starting at the next available file
	logTime("Writing states to CSV files in " + x_dirname);
	int states_written = writeStatesToFile(nodes, x_dirname, next_x_file_num, states_per_file);
	logTime("Writing action distributions to CSV files in " + y_dirname);
	int action_distributions_written = writeActionDistributionsToFile(nodes, y_dirname, next_y_file_num, states_per_file);

	ASSERT(states_written == action_distributions_written, "Must write same number of states and action distributions");
	return states_written;

}








// reads states from a CSV file, creating nodes out of each one, and storing them in the nodes vector
int readCSVFile(string game, int num_states, string file_path, vector<MCTS_Node*>* nodes, const ArgMap& options) {


	string line;
	ifstream f (file_path);
	int states_read = 0;
	while(getline(f, line)) {

		// do not read more states than instructed
		if (states_read == num_states) {
			break;
		}

		// process the CSV line into a state
		EnvState* state = stateFromCSVString(game, line, options);

		// grab node options
		// these calls to getInt, getString, getDouble, getBool, etc, assert that the necessary options are present
    	bool is_root = true;
    	int num_simulations = options.getInt("num_simulations", DEFAULT_NUM_SIMULATIONS);
    	int sample_actions = options.getBool("sample_actions", DEFAULT_SAMPLE_ACTIONS);
    	bool requires_nn = options.getBool("requires_nn", DEFAULT_REQUIRES_NN);
    	bool use_rave = options.getBool("use_rave", DEFAULT_USE_RAVE);
    	double c_b = options.getDouble("c_b", DEFAULT_C_B);
    	double c_rave = options.getDouble("c_rave", DEFAULT_C_RAVE);
    	double w_a = options.getDouble("w_a", DEFAULT_W_A);

    	// build an MCTS_Node from this state
    	MCTS_Node* node = new MCTS_Node(state, is_root, num_simulations, sample_actions, requires_nn, use_rave, c_b, c_rave, w_a);
    	// add the node to the vector of nodes
    	nodes->push_back(node);

    	states_read++;
    }

	return states_read;
}


// reads nodes from all the CSVs in a directory, stores them in the nodes vector
int readInputData(string game, int num_states, string input_data_path, int start_at, vector<MCTS_Node*>* nodes, const ArgMap& options) {

	ASSERT(nodes != NULL, "Nodes vector is NULL");
	ASSERT(num_states >= 0, "Num states is negative");
	ASSERT(GAME_OPTIONS.count(game) > 0, "Game " << game << " is not available");

	logTime("Reading in " + to_string(num_states) + " input states from " + input_data_path + " for the game " + game);

	// grab all the CSV files in the input directory (sorted by name)
	vector<string> input_filenames = csvFilesInDir(input_data_path, start_at);
	
	// read in states from each one, making sure not to read more than NUM_STATES in total
	int num_states_left = num_states;
	for (string filename : input_filenames) {
		if (num_states_left <= 0) {
			break;
		}
		int num_states_read = readCSVFile(game, num_states_left, filename, nodes, options);
		logTime("Read " + to_string(num_states_read) + " states from " + filename);
		num_states_left -= num_states_read;
	}

	// make sure that enough states were read
	ASSERT(nodes->size() <= num_states, "Do not read more than " << num_states << " states");

	return num_states - num_states_left;

}

