#ifndef MCTS_H

#include "hex_state.h"
#include "tictactoe.h"

#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <random>



using namespace std;

class StateVector {
public:
	int y;
	StateVector(int y);

	// new 
	vector<int>* squares;
	StateVector(vector<int>* squares);
	vector<int>* asVector();
};


class ActionDistribution {
public:
	int z;
	ActionDistribution(int z);

	// new 
	vector<int>* action_dist;
	ActionDistribution(vector<int>* action_dist);
	vector<int>* asVector();
};



class MCTS_Node {
public:
	int init_x;
	int x;
	MCTS_Node(int x);
	MCTS_Node();
	MCTS_Node* setX(int x);
	bool isComplete();
	int num_queries_required, num_queries_performed;



	// experimental below

	MCTS_Node(Tictactoe* state, bool is_root=true, int num_simulations=1000);
	bool isTerminal();
	bool isRoot();
	int numSimulationsFinished();
	bool simulationsFinished();
	int getDepth();
	MCTS_Node* setDepth(int depth);
	bool neverSubmittedToNN();
	void submittedToNN();
	bool awaitingNNResults();
	void receivedNNResults();
	StateVector* makeStateVector();
	void setActionDistribution(ActionDistribution* ad);
	MCTS_Node* chooseBestAction();

	MCTS_Node* getParent();
	MCTS_Node* getChild(int k);
	MCTS_Node* makeChild(int k);
	MCTS_Node* setParent(MCTS_Node* parent);
	MCTS_Node* setChild(MCTS_Node* child, int k);
	int childIndex();
	MCTS_Node* setChildIndex(int k); // marks this node as the k-th child of its parent

	int bestLegalAction(vector<double>* action_dist);
	int sampleLegalAction(vector<double>* action_dist);


	int N(); // n(s)
	int N(int a); // n(s,a)
	double R(int a);

	int NRave(); // n(s)-rave
	int NRave(int a); // n(s,a)-rave
	double RRave(int a); // r(s,a)-rave

	vector<double>* computeUCT();
	vector<double>* computeActionScores();

	StateVector* getStateVector();

	vector<int>* getActionCounts();

	MCTS_Node* sampleActions(bool sample_actions);

	void printStats();
	void printNode(string message);
	void printUCT();

	MCTS_Node* getRoot();
	MCTS_Node* setRoot(MCTS_Node* root);



	HexState* hex_state;
	Tictactoe* state;
	StateVector* state_vector;

	bool is_root;
	int num_simulations_finished;
	int total_num_simulations;
	int depth;
	bool submitted_to_nn;
	bool received_nn_results;

	ActionDistribution* nn_result;

	MCTS_Node* root;
	MCTS_Node* parent;
	vector<MCTS_Node*>* children;
	int child_index;

	int num_actions;

	int num_node_visits; // n(s)
	vector<int>* num_edge_traversals; // n(s,a)
	vector<double>* edge_rewards; // r(s, a)

	int num_node_visits_rave; // n(s)-rave
	vector<int>* num_edge_traversals_rave; // n(s,a)-rave
	vector<double>* edge_rewards_rave; // r(s,a)-rave

	double c_b = 1.0; // 0.25 for Vanilla MCTS, 0.05 for N-MCTS or N-MCTS (with policy and value)
	double eps = 1e-8; // to prevent division by 0

	string action_value_metric = "uct"; // uct, nn_dist, uct_nn, uct_rave

	mt19937 rng;
	bool sample_actions; // if true, sample actions with softmax probabilities, if false, choose action with highest score

private:
	void assertValidActionNum(int action_num);




};



/* Reads in serialized states from the given file, deserializes them, and creates a new MCTS_Node for each state.
 * Populates the given vector with these nodes. 
 * This function dispatches the heavy lifting to processLine, which processes a single line of the state file.
 */
void readStates(string infile, vector<MCTS_Node*>* node_vec);

/* Reads in a single line of the state file, deserializes it, creates an MCTS_Node, and places it in the appropriate slot of the given node vector.
 */
void processLine(string line, int state_num, vector<MCTS_Node*> *node_vec);







// adds the number in the action distribution to the x value of the given node. Returns a state vector with the new number
StateVector* processMCTSNode(MCTS_Node* node, ActionDistribution* ad);


// experimental below

MCTS_Node* runMCTS(MCTS_Node* node, ActionDistribution* ad, int max_depth);
MCTS_Node* propagateStats(MCTS_Node* node);
MCTS_Node* rolloutSimulation(MCTS_Node* node);

MCTS_Node* runAllSimulations(MCTS_Node* node, ActionDistribution* ad, int max_depth);






#endif