#include "mcts.h"

#include <iostream>
#include <iomanip>      // std::setprecision

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
#include <math.h> // log
#include <random>
#include <cfloat>       // std::numeric_limits





using namespace std;

double SOFTMAX_BASE = 2.7; // e?

std::random_device global_rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 global_rng(global_rd()); //Standard mersenne_twister_engine seeded with rd()

bool print_enabled = false;

void print(string message) {
	if (print_enabled) {
		cout << message << endl;
	}
}

StateVector::StateVector(int y) {
	this->y = y;
}

ActionDistribution::ActionDistribution(int z) {
	this->z = z;
}


void processLine(string line, int state_num, vector<MCTS_Node*> *node_vec) {
	try {
		// this will be replaced by code to parse a full state vector line
    	node_vec->at(state_num) = new MCTS_Node(stoi(line));
  	}
  	catch (int e) {
    	cout << "Line from state-vector data file could not be parsed - exception #" << e << '\n';
  	}
}


// this whole function will be replaced by code to deserialize state vectors (protobufs)?
void readStates(string infile, vector<MCTS_Node*>* node_vec) {

	string line;
	ifstream f (infile);
	int state_num = 0;
	while(getline(f, line)) {
    	processLine(line, state_num, node_vec);
    	state_num++;
    }
}


StateVector* processMCTSNode(MCTS_Node* node, ActionDistribution* ad) {
	if (node->isComplete()) {
		return new StateVector(node->x);
	}

	int new_x = ad->z + node->x;
	node->setX(new_x);
	if (ad->z != 0) {
		node->num_queries_performed++;
	}

	// attempt setting the node in the queue to a new node (works)
	MCTS_Node temp_node = *node;
	*node = temp_node;

	return new StateVector(new_x);
}







MCTS_Node::MCTS_Node(int x) {
	this->x = x;
	this->init_x = x;
	this->num_queries_required = x % 8;
	this->num_queries_performed = 0;
}

bool MCTS_Node::isComplete() {
	return num_queries_performed == num_queries_required;
}

MCTS_Node::MCTS_Node() {

}

MCTS_Node* MCTS_Node::setX(int x) {
	this->x = x;
	return this;
}





// experimental below

StateVector::StateVector(vector<int>* squares) {
	this->squares = squares;
}
vector<int>* StateVector::asVector() {
	return this->squares;
}

ActionDistribution::ActionDistribution(vector<int>* action_dist) {
	this->action_dist = action_dist;
}

vector<int>* ActionDistribution::asVector() {
	return this->action_dist;
}

MCTS_Node::MCTS_Node(Tictactoe* state, bool is_root, int num_simulations) {
	this->is_root = is_root;
	this->state = state;
	this->total_num_simulations = num_simulations;
	this->num_simulations_finished = 0;
	this->depth = 0;
	this->parent = NULL;
	if (is_root) {
		this->root = this;
	} else {
		this->root = NULL;
	}
	this->child_index = -1;
	this->children = new vector<MCTS_Node*>(25, NULL);
	this->submitted_to_nn = false;
	this->received_nn_results = false;

	this->num_actions = state->numActions();
	num_node_visits = 0; num_node_visits_rave = 0;
	num_edge_traversals = new vector<int>(num_actions, 0); num_edge_traversals_rave = new vector<int>(num_actions, 0);
	edge_rewards = new vector<double>(num_actions, 0.0); edge_rewards_rave = new vector<double>(num_actions, 0.0);

	this->state_vector = new StateVector(state->makeStateVector());

	this->sample_actions = true;

}

MCTS_Node* MCTS_Node::sampleActions(bool sample_actions) {
	this->sample_actions = sample_actions;
	return this;
}

bool MCTS_Node::isTerminal() {
	return this->state->isTerminalState();
}

bool MCTS_Node::isRoot() {
	return this->is_root;
}

int MCTS_Node::numSimulationsFinished() {
	return this->num_simulations_finished;
}

bool MCTS_Node::simulationsFinished() {
	return this->num_simulations_finished >= this->total_num_simulations;
}

int MCTS_Node::getDepth() {
	return this->depth;
}

MCTS_Node* MCTS_Node::setDepth(int depth) {
	this->depth = depth;
	return this;
}

bool MCTS_Node::neverSubmittedToNN() {
	return !this->submitted_to_nn;
}

void MCTS_Node::submittedToNN() {
	this->submitted_to_nn = true;
}

bool MCTS_Node::awaitingNNResults() {
	return !this->received_nn_results;
}

void MCTS_Node::receivedNNResults() {
	this->received_nn_results = true;
}

void MCTS_Node::setActionDistribution(ActionDistribution* ad) {
	this->nn_result = ad;
}

StateVector* MCTS_Node::makeStateVector() {
	return this->state_vector;
}

StateVector* MCTS_Node::getStateVector() {
	return this->state_vector;
}

vector<int>* MCTS_Node::getActionCounts() {
	return this->num_edge_traversals;
}





MCTS_Node* MCTS_Node::getParent() {
	return this->parent;
}

MCTS_Node* MCTS_Node::setParent(MCTS_Node* parent) {
	this->parent = parent;
	return this;
}

MCTS_Node* MCTS_Node::getChild(int k) {
	return this->children->at(k);
}

MCTS_Node* MCTS_Node::setChild(MCTS_Node* child, int k) {
	this->children->at(k) = child;
	return this;
}

MCTS_Node* MCTS_Node::makeChild(int k) {
	if (this->isTerminal()) {
		return this;
	}
	if (this->getChild(k) != NULL) {
		return this->getChild(k);
	}

	MCTS_Node* child_node = new MCTS_Node(this->state->nextState(k), false); // is_root is false
	child_node->setParent(this);
	child_node->setChildIndex(k);
	child_node->setDepth(this->depth + 1);
	child_node->setRoot(this->root);
	child_node->sampleActions(this->sample_actions);

	this->setChild(child_node, k);

	// increment depth of child
	return child_node;
}

MCTS_Node* MCTS_Node::setChildIndex(int k) {
	this->child_index = k;
	return this;
}

int MCTS_Node::childIndex() {
	return child_index;
}

MCTS_Node* MCTS_Node::getRoot() {
	return this->root;
}

MCTS_Node* MCTS_Node::setRoot(MCTS_Node* root) {
	this->root = root;
	return this;
}


void MCTS_Node::assertValidActionNum(int action_num) {
	if (action_num < 0 || action_num >= this->num_actions) {
		throw invalid_argument("Illegal action: " + to_string(action_num));
	}
}

int MCTS_Node::N() {
	return this->num_node_visits;
}
int MCTS_Node::N(int a) {
	assertValidActionNum(a);
	return this->num_edge_traversals->at(a);
}
double MCTS_Node::R(int a) {
	assertValidActionNum(a);
	return this->edge_rewards->at(a);
}

int MCTS_Node::NRave() {
	return this->num_node_visits_rave;
}
int MCTS_Node::NRave(int a) {
	assertValidActionNum(a);
	return this->num_edge_traversals_rave->at(a);
}
double MCTS_Node::RRave(int a) {
	assertValidActionNum(a);
	return this->edge_rewards_rave->at(a);
}


void printDoubleVector(vector<double>* vec, string name) {
	cout << name << ":\t\t\t";
	for (double d : *vec) {
		cout << setprecision(3) << d << "\t\t\t";
	}
	cout << endl;
}


void printIntVector(vector<int>* vec, string name) {
	cout << name << ":\t\t\t";
	for (int d : *vec) {
		cout << d << "\t\t\t";
	}
	cout << endl;
}


vector<double>* MCTS_Node::computeUCT() {

	vector<double>* action_scores = new vector<double>(this->num_actions, 0);
	
	// if never been to this node, return all 0s (uniform)
	if (N() == 0) {
		return action_scores;
	}

	double avg_reward_term;
	double avg_log_visits_term;
	double score;

	vector<double>* avg_rewards = new vector<double>(this->num_actions, 0);
	vector<double>* avg_log_visits = new vector<double>(this->num_actions, 0);

	for (int a = 0; a < this->num_actions; a++) {

		if (N(a) == 0) {
			avg_reward_term = 0;
		} else {
			int turn = this->state->turn();
			avg_reward_term = (R(a) * turn)/N(a); // make sure we "minimize reward" (maximize negative reward if it is player 2 (player -1)'s turn)
		} 

		avg_log_visits_term = log(N()) / (N(a) + 1);
		score = avg_reward_term + (this->c_b * sqrt(avg_log_visits_term));
		action_scores->at(a) = score;

		avg_rewards->at(a) = avg_reward_term;
		avg_log_visits->at(a) = avg_log_visits_term;
	}

	//printDoubleVector(avg_rewards, "R avg in func");
	//printDoubleVector(avg_log_visits, "avg log visit");

	return action_scores;
}

vector<double>* MCTS_Node::computeActionScores() {
	if (this->action_value_metric == "uct") {
		return this->computeUCT();
	}

	return new vector<double>(this->num_actions, 0);
}

double randomDouble(double lower_bound, double upper_bound) {
	uniform_real_distribution<> dist(lower_bound, upper_bound);
	return dist(global_rng);
}



// eventually should use a global rng
int sampleProportionalToWeights(vector<double>* weights) {
	vector<double>* thresholds = new vector<double>(weights->size(), 0);

	double accum_sum = 0;
	for (int pos = 0; pos < weights->size(); pos++) {
		accum_sum += weights->at(pos);
		thresholds->at(pos) = accum_sum;
	}

	double r = randomDouble(0, accum_sum);
	print("R: " + to_string(r));
	for (int pos = 1; pos < weights->size() + 1; pos++) {
		if (r <= thresholds->at(pos - 1)) {
			return pos - 1;
		}
	} 

	cout << "Error: cannot properly sample a legal action!" << endl;
	exit(1);
}

int MCTS_Node::sampleLegalAction(vector<double>* action_dist) {

	vector<double>* softmax_numerators = new vector<double>(this->num_actions, 0);
	double softmax_denominator = 0;

	double raw_score, exp_score;
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		
		raw_score = action_dist->at(action_num);

		if (this->state->isLegalAction(action_num)) {
			exp_score = pow(SOFTMAX_BASE, raw_score);
			softmax_numerators->at(action_num) = exp_score;
			softmax_denominator += exp_score;
		
		}
	}

	vector<double>* softmax_scores = new vector<double>(this->num_actions, 0);
	for (int action_num = 0; action_num < this->num_actions; action_num++) {
		softmax_scores->at(action_num) = softmax_numerators->at(action_num) / softmax_denominator;
	}
	//printDoubleVector(softmax_scores, "softmax");

	return sampleProportionalToWeights(softmax_scores);

    
}

int MCTS_Node::bestLegalAction(vector<double>* action_dist) {

	double best_action_score = -1 * DBL_MAX;
	int best_action_num = 0;

	for (int action_num = 0; action_num < action_dist->size(); action_num++) {
		if (action_dist->at(action_num) > best_action_score && this->state->isLegalAction(action_num)) {
			best_action_score = action_dist->at(action_num);
			best_action_num = action_num;
		}
	}
	return best_action_num;
}

MCTS_Node* MCTS_Node::chooseBestAction() {

	if (this->isTerminal()) {
		return this;
	}

	this->printNode("Choosing action for state. Depth=" + to_string(this->getDepth()));
	vector<double>* action_scores = this->computeActionScores();
	int best_action;
	if (this->sample_actions) {
		best_action = sampleLegalAction(action_scores);
	} else {
		print("taking best legal action");
		best_action = bestLegalAction(action_scores);
	}
	print("Chose action " + to_string(best_action) + "\n");
	
	
	MCTS_Node* child_node = this->makeChild(best_action);
	return child_node;
}





MCTS_Node* propagateStats(MCTS_Node* node) {

	// the given node is terminal, start here and propagate up
	double reward = ((double) node->state->reward());
	MCTS_Node* curr_node = node;

	int child_index = curr_node->childIndex();

	while (curr_node != NULL) {

		// there are no stats for the terminal state
		if (curr_node->isTerminal()) {
			curr_node = curr_node->getParent();
			continue;
		}

		curr_node->num_node_visits += 1;
		curr_node->num_edge_traversals->at(child_index) += 1;
		curr_node->edge_rewards->at(child_index) += reward;

		if (curr_node->isRoot()) {
			// update the number of finished simulations for this root node
			curr_node->num_simulations_finished += 1;
			return curr_node;
		}

		child_index = curr_node->childIndex();
		curr_node = curr_node->getParent();

	}
	
	return curr_node;
}


MCTS_Node* rolloutSimulation(MCTS_Node* node) {

	// choose a random action, take it. repeat until terminal action, return terminal action
	MCTS_Node* curr_node = node;
	while (!curr_node->isTerminal()) {
		int random_action = curr_node->state->randomAction();
		curr_node = curr_node->makeChild(random_action);
	}
	return curr_node;
}


void MCTS_Node::printNode(string message) {
	if (!print_enabled) return;
	cout << message << endl;
	this->state->printBoard();
	this->printStats();
}

void MCTS_Node::printUCT() {
	vector<double>* uct = this->computeUCT();
	for (int pos = 0; pos < uct->size(); pos++) {
		if (!this->state->isLegalAction(pos)) {
			uct->at(pos) = 0;
		}
	}
	printDoubleVector(uct, "UCT()()");
}

void MCTS_Node::printStats() {
	printIntVector(this->num_edge_traversals, "N(S, A)");
	printDoubleVector(this->edge_rewards, "R(S, A)");
	vector<double>* avg_rewards = new vector<double>(this->num_actions, 0);
	double n, r;
	for (int pos = 0; pos < this->num_actions; pos++) {
		n = this->num_edge_traversals->at(pos);
		r = this->edge_rewards->at(pos);
		if (n != 0) {
			avg_rewards->at(pos) = ((double) r) / n;
		}
	}
	printDoubleVector(avg_rewards, "Avg rew");
	printUCT();
}


MCTS_Node* runMCTS(MCTS_Node* node, ActionDistribution* ad, int max_depth) {

	MCTS_Node* curr_node = node;

	int t = 0;
	while (true) {
		assert(curr_node != NULL);

		// if we are at the root and have finished all the simulations, break
		if (curr_node->isRoot() && curr_node->simulationsFinished()) {
			break;
		}

		// if terminal state, do stats propagation back up to the root
		if (curr_node->isTerminal()) {

			double reward = ((double) curr_node->state->reward());
			curr_node = propagateStats(curr_node); // should return root node
			print("Reward for simulation " + to_string(curr_node->numSimulationsFinished() -1) + ": " + to_string(reward) + "\n\n");
			print("BEGIN SIMULATION " + to_string(curr_node->numSimulationsFinished()));

			continue;
		}

		// if we are at max depth, perform rollout
		if (curr_node->getDepth() == max_depth) {
			curr_node = rolloutSimulation(curr_node); // should return terminal node
			continue;
		}

		// if at an intermediate node, look to see if the NN action distribution is already fetched
		// if never submitted, submit self and wait.  
		if (curr_node->neverSubmittedToNN()) {
			curr_node->submittedToNN();
			//*node = *curr_node;
			return curr_node;
		}
		// if was previously waiting for AD, grab the given one and set it as a field
		if (curr_node->awaitingNNResults()) {
			curr_node->receivedNNResults();
			curr_node->setActionDistribution(ad);
		}

		// get the best child and iterate
		
		curr_node = curr_node->chooseBestAction();
		
	}

	//*node = *curr_node;
	return curr_node;
}


MCTS_Node* runAllSimulations(MCTS_Node* node, ActionDistribution* ad, int max_depth) {
	while (!node->simulationsFinished()) {
		node = runMCTS(node, ad, max_depth);
	}
	return node;
}






