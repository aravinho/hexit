#ifndef MCTS_NODE_H

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