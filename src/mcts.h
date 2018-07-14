#ifndef MCTS_H

#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>


using namespace std;

class MCTS_Node {
public:
	int init_x;
	int x;
	MCTS_Node(int x);
	MCTS_Node();
	MCTS_Node* setX(int x);
	bool isComplete();
	int num_queries_required, num_queries_performed;
	
};



/* Reads in serialized states from the given file, deserializes them, and creates a new MCTS_Node for each state.
 * Populates the given vector with these nodes. 
 * This function dispatches the heavy lifting to processLine, which processes a single line of the state file.
 */
void readStates(string infile, vector<MCTS_Node*>* node_vec);

/* Reads in a single line of the state file, deserializes it, creates an MCTS_Node, and places it in the appropriate slot of the given node vector.
 */
void processLine(string line, int state_num, vector<MCTS_Node*> *node_vec);




class StateVector {
public:
	int y;
	StateVector(int y);
};

class ActionDistribution {
public:
	int z;
	ActionDistribution(int z);
};


// adds the number in the action distribution to the x value of the given node. Returns a state vector with the new number
StateVector* processMCTSNode(MCTS_Node* node, ActionDistribution* ad);





#endif