#ifndef MCTS_H

#include "env_state.h"
#include "hex_state.h"
#include "tictactoe.h"
#include "config.h"
#include "utils.h"

#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <random>
#include <math.h> // log



using namespace std;

class MCTS_Node;
class StateVector;
class ActionDistribution;


// make dummy
// modify so vector of doubles

/**
 * This class is a wrapper around the type of vector that a neural network would take as input.
 */

class StateVector {

public:

	/* Creates a state vector with an empty _sv */
	StateVector();

	/* Creates a state vector, and copies the contents of the given vec into this->_sv. */
	StateVector(const vector<double>& vec);

	/* Returns the K-th element of the _sv vector. 
	 * Errors if K is out of range. */
	double at(int k) const;

	/* Returns a CSV string representation of this StateVector. */
	string asCSVString() const;

	/* Destructor has nothing to do. */
	~StateVector();

private:
	vector<double> _sv;
};


// dummy
class ActionDistribution {
public:
	int z;
	ActionDistribution(int z);

	// dummy
	ActionDistribution();

	// new 
	vector<double>* action_dist;
	ActionDistribution(vector<double>* action_dist);
	ActionDistribution(int num_actions, string csv_line, char delimiter=',');
	vector<double>* asVector();

	/* Returns the K-th element of the action_dist vector.
	 * Errors if K is out of range. */
	double at(int k) const;

	/* Returns a CSV string representation of this ActionDistribution. */ // to do
	string asCSVString() const;

	~ActionDistribution();
};


// To do: make public getters for state, and updaters for N, R, etc
class MCTS_Node {

public:	


	/**
	 * Initializes an MCTS_Node with the given EnvState.
	 * All private instance variables are given logical initial values.
	 * Creates and stores the StateVector for the state.
	 * The flag IS_ROOT specifies whether this node is the root of an MCTS tree.
	 * NUM_SIMULATIONS (only relevant if root) specifies the number of simulations to run starting from this node.
	 * The flag SAMPLE_ACTIONS specifies whether MCTS should sample actions proportional to the sofmtax of scores (UCT/Rave),
	 * or simply choose the action with the highest score.
	 * The flag REQUIRES_NN specified whether this node performs MCTS or Neural-MCTS.
	 * If false, it does not need to query an NN apprentice when choosing an action.
	 * The flag USE_RAVE specifies whether to use Rapid Value Estimation (RAVE) when choosing actions.
	 * See comments for NRave and RRave functions for details.
	 */
	MCTS_Node(EnvState* state, bool is_root=true, int num_simulations=DEFAULT_NUM_SIMULATIONS, bool sample_actions=DEFAULT_SAMPLE_ACTIONS, bool requires_nn=DEFAULT_REQUIRES_NN, bool use_rave=DEFAULT_USE_RAVE,
		double c_b=DEFAULT_C_B, double c_rave=DEFAULT_C_RAVE, double w_a=DEFAULT_W_A);

	/**
	 * Deletes all of the N and R vectors for this node.
	 * Does NOT delete this->state, this->state_vector or this->nn_result, because this could cause the calling code 
	 * in the ThreadManager to be messed up.
	 * This function also recursively calls the destructor on all of its non-null children.
	 */
	~MCTS_Node();

	/**
	 * This function is meant to be called on root nodes only (errors otherwise).
	 * It does not delete itself, but recursively calls the destructor on all of its non-null children.
	 * This way, the rest of the tree gets deleted, but not the root.
	 * This function is used to free up memory space once all simulations are done, but because we need the root nodes to stay alive.
	 */
	void deleteTree();



	/* Returns true if this node's state is a terminal state, and false otherwise. */
	bool isTerminal() const;

	/* Returns true if this node is a root of an MCTS tree, and false otherwise. */
	bool isRoot() const;

	/**
	 * Only relevant if this node is a root node.
	 * Returns true if this root node has finished all the simulations it was meant to, and false otherwise. */
	bool simulationsFinished() const;

	/* Mark that another simulation has finished (increment this->num_simulations_finished). */
	void markSimulationFinished();



	
	/* Returns whether this node uses Rapid Value Estimation (RAVE). */
	bool usesRave() const;

	/* Returns whether this node requires a Neural Net apprentice when choosing actions. */
	bool requiresNN() const;

	/**
	 * Returns true if this node requires an NN, AND has not yet submitted its state vector to the NN.
	 * Returns false if this node doesn't use an NN, OR if the node has previously submitted to the NN.
	 */
	bool neverSubmittedToNN() const;

	/* Mark this node as having submitted its state vector to the NN. */ 
	void markSubmittedToNN();

	/**
	 * Returns true if this node requires an NN, but has not set its nn_result (The action distribution recommended by the NN).
	 * Returns false if this node doesn't use an NN, or if it has already set its nn_result field (with setActionDistribution).
	 */
	bool awaitingNNResults() const;

	/* Mark this node as having set its nn_result field. */
	void markReceivedNNResults();


	/* Returns this->num_actions. */
	int getNumActions() const;

	/* Returns the depth of this node in its tree. (Root nodes have a depth of 0). */
	int getDepth() const;

	/* Returns the parent of this node (Root nodes have a NULL parent). */
	MCTS_Node* getParent() const;

	/* Returns the K-th child of this node, which may be NULL if this node has never visited its K-th child. */
	MCTS_Node* getChild(int k) const;

	/* Returns the child_index of this node.  If this node is the K-th child of its parent, then its child_index is K. */
	int getChildIndex() const;

	/* Returns the root of this node's tree.  If this node is a root, returns itself. */
	MCTS_Node* getRoot() const;

	/**
	 * Returns the K-th child of this node, creating it if it hasn't yet been created.
	 * If this node's state is terminal, returns itself.
	 * If this node's K-th child has already been made, just returns that child.
	 *
	 * Otherwise, grabs the state that results from taking action K from this node's state, and creates a node for that new state
	 * Sets that node to be the K-th child of this one (and sets the parent of that new node to be this node).
	 * Sets all the instance variables of the new node appropriately (the depth, for example, is one more than the depth of this node.)
	 * Returns the new node.
	 */
	MCTS_Node* makeChild(int k);





	/* Returns the vector of edge traversals. */
	vector<int>* edgeTraversals() const;

	/* Returns the vector of edge rewards. */
	vector<double>* edgeRewards() const;

	/* Returns the vector of edge traversals for RAVE. */
	vector<int>* edgeTraversalsRave() const;

	/* Returns the vector of edge rewards for RAVE. */
	vector<double>* edgeRewardsRave() const;




	/* Return a pointer to the EnvState instance (the state) of this node. */
	EnvState* getState() const;

	/* Return a pointer to a StateVector instance that acts as a wrapper around the state vector of this node's state. */
	StateVector* getStateVector() const;

	/* Set the nn_result field with the given ActionDistribution. */
	void setNNActionDistribution(ActionDistribution* ad);

	/* Returns a vector where the K-th element is the number of times action K was taken from this node during MCTS. */
	vector<int>* getActionCounts() const;

	/**
	 * Populates a vector where the K-th element is the proportion of times action K was taken from this node during MCTS. 
 	 * The action_dist vector must not be null, and must have at least this->num_actions elements.
	 */
	void getActionDistribution(vector<double>* action_dist) const;




	/**
	 * Choose the best action (from previous simulation stats and/or NN apprentice advice) to take from this node's state.
	 * Does this by first computing scores for each action (UCT, UCT-NN, UCT-Rave, UCT-NN-Rave, etc).
	 * Then either samples an action with probabilities proportional to the softmax of the scores (if this->sample_actions is true),
	 * or else picks the action with the highest score.
	 *
	 * Suppose the above logic chooses action K as the best action.
	 * This function will then return the K-th child node, creating that node if it has never been visited before.
	 */
	MCTS_Node* chooseBestAction();



	/**
	 * Updates the necessary stats for a simulation in which this node took the given action, which eventually resulted in the given reward.
	 * This function is used when the node does not use RAVE. 
	 * Increments this->num_node_vists and this->num_edge_traversals(action_num) by 1, and increment this->edge_rewards(action_num) by REWARD.
	 * CHOSEN_ACTION is expected to be within the correct range, and it is an error if it is not.
	 */
	void updateStats(int chosen_action, double reward, bool update_rave_stats=false);

	/**
	 * Updates the necessary stats for this node, using the RAVE all-moves-as-first method.
	 * this->num_edge_traversals_rave and this->num_node_visits_rave are both incremented by 1 for every action in the given list.
	 * this->edge_rewards_rave is incremented by REWARD for every action in the given list.
	 */
	void updateStatsRave(const vector<int>& chosen_actions, double reward);

	




private:

	/**
	 * Only relevant if this node is a root node.
	 * Returns the number of completed tree-search simulations that have occurred from this root node. */
	int numSimulationsFinished() const;



	/* Sets the depth of this node to the given value.  The value must be a nonnegative integer, or the function errors. */
	MCTS_Node* setDepth(int depth);

	/* Sets the parent of this node to the given node, and returns this node. Errors if the parent is NULL. */
	MCTS_Node* setParent(MCTS_Node* parent);

	/* Sets the K-th child of this node to the given node, and returns this node. Errors if the child is NULL, or K is out of range. */
	MCTS_Node* setChild(MCTS_Node* child, int k);

	/* Set the child_index of this node to K. This denotes that this node is the K-th child of its parent. Returns this node. Errors if K is out of range. */
	MCTS_Node* setChildIndex(int k);

	/* Sets the root pointer of this node to the given node. Returns this node. Errors if root is NULL. */
	MCTS_Node* setRoot(MCTS_Node* root);





	/* Returns the number of times this node has been visited during MCTS. */
	int N() const; // n(s)

	/* Returns the number of times action A has been taken from this node during MCTS.*/
	int N(int a) const; // n(s,a)

	/* Returns the total reward over all simulations in which we took action A from this node. */
	double R(int a) const; // r(s, a)

	 /* Returns the number of times this node has been visited during MCTS, using the RAVE formula. */ 
	int NRave() const; // n(s)

	/* Returns the number of times action A has been taken from this node during MCTS, using the RAVE formula. */
	int NRave(int a) const; // n(s,a)

	/* Returns the total reward over all simulation in which we took action A from this node, using the RAVE formula. */
	double RRave(int a) const; // r(s, a)







	/** A note on some formulas used to compute action scores:

	UCT(s, a) = r(s,a)/n(s,a) + c_b * sqrt(log n(s)/ n(s,a)), where c_b ~ 0.05 (0.25 for Vanilla non-neural)
	
	UCT_RAVE(s, a) = r_rave(s,a)/n_rave(s,a) + c_b * sqrt(log n_rave(s) / n_rave(s, a))
	beta(s, a) = sqrt(c_rave / (3n(s) + c_rave)), where c_rave ~ 3000
	UCT_U,RAVE(s,a) = beta(s,a) * UCT_RAVE(s,a) + (1 - beta(s,a)) * UCT(s,a)

	UCT_NN(s, a) = UCT(s,a) + w_a * pi(a|s) / n(s,a) + 1, where w_a ~ sqrt(num_simulations), and pi(a|s) is the NN prediction
	UCT_NN_RAVE(s, a) = UCT_U,RAVE(s, a) w_a  * pi(a|s) / n(s,a) + 1

	*/



	/**
	 * Computes scores for each action using the basic UCT formula.
	 * The given vector is populated with these scores.
	 * Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	 */
	void computeUCT(vector<double>* action_scores) const;

	/**
	 * Computes scores for each action using the UCT_RAVE formula.
	 * The given vector is populated with these scores.
	 * Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	 */
	void computeUCT_Rave(vector<double>* action_scores) const;

	/**
	 * Computes scores for each action using the UCT_U_RAVE formula.
	 * The given vector is populated with these scores.
	 * Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	 */
	void computeUCT_U_Rave(vector<double>* action_scores) const;

	/**
	 * Computes scores for each action using the UCT_NN formula.
	 * The given vector is populated with these scores.
	 * Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	 */
	void computeUCT_NN(vector<double>* action_scores) const;

	/**
	 * Computes scores for each action using the UCT_NN_RAVE formula.
	 * The given vector is populated with these scores.
	 * Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	 */
	void computeUCT_NN_Rave(vector<double>* action_scores) const;



	/**
	* Computes scores for each of the actions.
	* The scores are computed using one of the above formulas:
	*
	* if neither NN nor RAVE is used, UCT(s, a)
	* if NN but not RAVE, UCT_NN(s, a)
	* if RAVE but not NN, UCT_U,RAVE(s, a)
	* if NN and RAVE, UCT_NN_RAVE(s, a)
	*
	* The given vector is populated with these scores.
	* Errors if the given vector is a NULL pointer, or is not of size this->num_actions.
	*/
	void computeActionScores(vector<double>* action_scores) const;


	/**
	 * This method is passed a vector of scores, one for each of the actions.
	 * First, the softmaxes are computed for each scores (the scores for the illegal actions are given a softmax value of 0).
	 * Then an action is sampled, and returned, with probability proportional to the softmax values.
	 * It is expected that the number of elements in action_dist is this->num_actions. */
	int sampleLegalAction(const vector<double>& action_scores);


	/**
	 * This method is passed a vector of scores, one for each of the action.
	 * It returns the legal action that has the highest score.
	 * It is expected that the number of elements in action_dist is this->num_actions. */
	int bestLegalAction(const vector<double>& action_scores);


	/* The state that this node represents. */
	EnvState* state;
	/* A Neural Net representation of that state. */
	StateVector* state_vector;

	/* Denotes whether this node is the root of its tree. */
	bool is_root;
	/* Denotes the number of simulations this node has completed (only relevant if root node). */
	int num_simulations_finished;
	/* Denotes the total number of simulations this node must complete (only relevant if root node). */
	int total_num_simulations;
	/* Denotes this node's tree depth (root nodes have a depth of 0). */
	int depth;


	/* Denotes whether this node has submitted its state vector to the NN. */
	bool submitted_to_nn;
	/* Denotes whether this node has received results (an ActionDistribution) back from the NN. */
	bool received_nn_results;
	/* The result from the NN. */
	ActionDistribution* nn_result;
	/* The size of the action space of the state's environment. */
	int num_actions;


	/* A pointer to the root of this node's tree (if this is a root node, just a pointer to itself). */
	MCTS_Node* root;
	/* A pointer to the parent node of this node (if this is a root node, the parent is NULL). */
	MCTS_Node* parent;
	/* A vector of this node's children (one for each action that in state's environment). */
	vector<MCTS_Node*>* children;
	/* If this node is the K-th child of its parent, its child_index is K.  (The child_index of a root node is -1). */
	int child_index;


	/* The number of times this node has been visited during MCTS. */
	int num_node_visits; // n(s)
	/* The number of times action A has been taken from this state during MCTS. */
	vector<int>* num_edge_traversals; // n(s,a)
	/* The total reward over all simulation in which action A was taken from this state during MCTS. */
	vector<double>* edge_rewards; // r(s, a)


	/* The same statistics as above, but using the Rapid Value Estimation formula (RAVE). */
	int num_node_visits_rave; // n(s)-rave
	vector<int>* num_edge_traversals_rave; // n(s,a)-rave
	vector<double>* edge_rewards_rave; // r(s,a)-rave


	/* Hyperparameter that weighs exploration vs taking the best actions. */
	double c_b;
	/* Hyperparameter that determines how quickly RAVE values are down-weighted as the number of samples increases. */
	double c_rave;
	/* Hyperparameter that determines how much weight to give the NN apprentice recommendation. */
	double w_a;


	/* If true, sample actions with softmax probabilities, if false, choose action with highest score. */
	bool sample_actions;
	/* Specifies whether an NN apprentice is needed when determining which action to take during MCTS. */
	bool requires_nn;
	/* Specifies whether to use Rapid Value Estimation (RAVE) when computing action scores. */
	bool use_rave;




};



/** A note on how node and edge statistics are updated.

If this node (S) does not use RAVE, and action A was taken from this node during the simulation, which resulted in an eventual reward of R:
N(S) += 1
N(S, A) += 1
R(S, A) += R

If the node uses RAVE, after a simulation (S1, A1, S2, A2, ... ST) which results in an eventual reward R, the statistics are updated as follows:
N_rave(S_ti, A_tj) += 1, for all t_i <= t_j such that S_ti and S_tj both corresponded to the same player's turn
R_rave(S_ti, A_tj) += R, for all t_i <= t_j such that S_ti and S_tj both corresponded to the same player's turn
N_rave(S_ti) = sum_a N_rav(S_ti, a), for all t_i

*/

/**
 * This function takes in a terminal node which represents a terminal state that marked the end of a simulation.
 * It performs stats updates at every node, starting from this one, following the path up to the root of the tree.
 * if this->use_rave, RAVE updates are performed in addition to regular updates.
 * Finally returns the root node.
 */
MCTS_Node* propagateStats(MCTS_Node* node);

/**
 * Starts at the given node, and randomly selects actions, progressing down the tree till a terminal state is reached.
 * Returns the node for this terminal state.
 */
MCTS_Node* rolloutSimulation(MCTS_Node* node);

/**
 * Runs a simulation starting from the given node and continuing till a terminal state.
 * Returns early if this node requires an NN apprentice and has never submitted to an NN.
 * If this node requires an NN, has submitted to an NN but has not received a result, it will set the given ActionDistribution as an instance variable.
 * It will thereafter not need to submit to the NN.
 *
 * If max_depth is reached, random action rollout begins.
 * Once a terminal state is reached, stats propagation is performed. The root node is returned, with all the nodes along the path
 * of the previous simulations having had their stats updated.
 *
 * Errors if NODE is null, or if max_depth is not a positive number.
 */
MCTS_Node* runMCTS(MCTS_Node* node, int max_depth, ActionDistribution* ad=NULL);

/**
 * Runs all the simulations for the given node.
 * Each simulation goes until the given max_depth then performs rollout.
 * This function is only used when the tree does not require an NN, since there is no ActionDistribution passed.
 * Returns the root node, after having finished all the simulations.
 *
 * Errors is NODE is null, or if max_depth is not a positive number.
 */
MCTS_Node* runAllSimulations(MCTS_Node* node, int max_depth=DEFAULT_MAX_DEPTH);








/** 
 * Populates the nodes vector with the data read in from the given INPUT_DATA_PATH.
 * The input_data_path specifies a directory with CSV files.
 * Each file contains many lines, each of which encodes a state for a particular game (Tictactoe, Hex, etc...) (given by GAME arg)
 * This method creates an appropriate EnvState instance (HexState, TictactoeState, etc) from the CSV string representation.
 * That state is then used to initialize an MCTS_Node, which will run NUM_SIMULATIONS simulations.
 *
 * Returns the number of nodes read.
 * Errors if game is not recognized (currently, limited to "hex" or "tictactoe"), or if the given vector of nodes is NULL.
 */
int readInputData(string game, int num_states, string input_data_path, vector<MCTS_Node*>* nodes, const ArgMap& options);

/**
 * Reads in at most NUM_STATES states (for the given GAME) from a single CSV file, given by the FILE_PATH.
 * Creates an MCTS_Node for each state, and populates the nodes vector with this node.
 * The given OPTIONS map is used when constructing the nodes.
 * Returns the number of lines read, which should be at most NUM_STATES.
 */
int readCSVFile(string game, int num_states, string file_path, vector<MCTS_Node*>* nodes, const ArgMap& options);






/**
 * Writes the given vector of nodes to CSV files in the given directory.
 * Writes the state vectors into base_dirname/states, and the action distributions into base_dirname/action_distributions.
 * Starts writing at the first available CSV file in the directory, and writes at most STATES_PER_FILE states per file.
 * Returns the number of nodes written.
 */
int writeBatchToFile(vector<MCTS_Node*>* nodes, string base_dirname, int states_per_file=DEFAULT_STATES_PER_FILE);


/**
 * Given a batch of root nodes, writes each node's state to a CSV file.
 * The files will be in the given directory, and this function begins writing at next_file_num.
 * It writes at most STATES_PER_FILE states in each file, moving onto the next file if necessary. 
 * Returns the number of states written.
 */
int writeStatesToFile(vector<MCTS_Node*>* nodes, string dirname, int next_file_num, int states_per_file=DEFAULT_STATES_PER_FILE);


/**
 * Given a batch of root nodes, writes each action distribution to a CSV file.
 * The files will be in the given directory, and this function begins writing at next_file_num.
 * It writes at most STATES_PER_FILE states in each file, moving onto the next file if necessary. 
 * Returns the number of action distributions written.
 */
int writeActionDistributionsToFile(vector<MCTS_Node*>* nodes, string dirname, int next_file_num, int states_per_file=DEFAULT_STATES_PER_FILE);









#endif