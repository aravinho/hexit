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




using namespace std;

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
	/*if (ad->z != 0 && ad->z != node->x) {
		cout << "YELL, node " << node->init_x << " is messed. ad->Z = " << ad->z << ",  node->x = " << node->x << endl;
	}*/
	int new_x = ad->z + node->x;
	node->setX(new_x);
	if (ad->z != 0) {
		node->num_queries_performed++;
	}
	return new StateVector(new_x);
}

