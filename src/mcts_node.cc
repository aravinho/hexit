#include "mcts_node.h"

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

StateVector::StateVector(int y) {
	this->y = y;
}

ActionDistribution::ActionDistribution(int z) {
	this->z = z;
}