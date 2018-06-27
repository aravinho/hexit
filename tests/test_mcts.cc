#include <iostream>
#include <vector>
#include <set>
#include <stdlib.h> // srand, rand
#include <time.h> // to seed RNG


#include "test_mcts.h"
#include "../src/tictactoe.h"
#include "../src/mcts.h"
#include "test_utils.h"



void assertTrueMCTS(bool cond, string test_name, string message) {
	string test_file = "test_mcts.cc";
	assertWithMessage(cond, test_file, test_name, message);
}

void simpleFlagFlip(MCTS_Shared_Data* data) {
	// all minibatches should initially be worker safe
	for (int minibatch_num = 0; minibatch_num < 4; minibatch_num++) {
		assertTrueMCTS(data->isWorkerSafe(minibatch_num), "sharedDataSimpleSimulation", "all minibatches must initially be worker safe");
		data->flipOwnershipFlag(minibatch_num);
		assertTrueMCTS(data->isMasterSafe(minibatch_num), "sharedDataSimpleSimulation", "all minibatches must now be master safe from worker");
	}
}

void sharedDataSimpleSimulation() {
	int num_nodes = 64, minibatch_size = 16, num_threads = 4;
	int nodes_per_thread = 16, num_minibatches = 4;

	cout << "num_minibatches" << num_minibatches << endl;

	vector<MCTS_Node*>* all_nodes = new vector<MCTS_Node*>(64);
	for (int node_num = 0; node_num < 64; node_num++) {
		all_nodes->at(node_num) = (new MCTS_Node())->setX(node_num);
	} 
	cout << "hi" << endl;
	MCTS_Shared_Data *data = new MCTS_Shared_Data(all_nodes);

	

	thread worker = thread(simpleFlagFlip, data);
	worker.join();

	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		assertTrueMCTS(data->isMasterSafe(minibatch_num), "sharedDataSimpleSimulation", "all minibatches must now be master safe from master");
	}

}


void runMctsTests() {
	// seed RNG
	srand(time(NULL));
	runPythonScript("data/nums.txt");
	//cout << "Testing MCTS..." << endl << endl;
	//cout << "Done Testing MCTS." << endl << endl;

}

void runMctsSharedDataTests() {
	cout << "Testing MCTS_Shared_Data class..." << endl << endl;
	sharedDataSimpleSimulation();
	cout << "Done testing MCTS_Shared_Data class." << endl << endl;
}
