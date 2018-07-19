#include <iostream>
#include <vector>
#include <set>
#include <stdlib.h> // srand, rand
#include <time.h> // to seed RNG
#include <chrono> // sleep
#include <thread>
#include <fstream>



#include "test_thread_manager.h"
//#include "../src/mcts.h"
#include "../src/mcts_thread_manager.h"
#include "test_utils.h"



void assertTrueThreadManager(bool cond, string test_name, string message) {
	string test_file = "test_thread_manager.cc";
	assertWithMessage(cond, test_file, test_name, message);
}

void workerFunc(int thread_num, MCTS_Thread_Manager* data) {

	// Register this thread for logging
	data->registerThreadName(this_thread::get_id(), "Worker " + to_string(thread_num));


	// iterate through all the active nodes in my thread
	// nextActiveNodeNum will return -1 when there are no more nodes for me to process
	int node_num;
	while ((node_num = data->nextActiveNodeNum(thread_num)) != -1) {
		// wait till it's safe to touch the minibatch that holds this node
		data->workerWaitForNode(node_num, thread_num);

		// grab the node, and its corresponding NN output (Action distribution) that the master has placed
		MCTS_Node* node = data->getNode(node_num);
		ActionDistribution* ad = data->getNNResult(node_num);
	
		// process the node till it must wait, and grab the state vector it submits
		// if the node is complete to begin with, this call will do nothing
		StateVector* state_vector = processMCTSNode(node, ad);

		// mark that this thread has processed this node, so it doesn't loop around and do so again until the master has refreshed its minibatch.
		data->markNodeProcessed(node_num, thread_num);

		// if this node was already complete, or is now complete, mark it complete
		// there is no need to submit any state vector
		if (node->isComplete()) {
			data->markNodeComplete(node_num);
			continue;
		}

		// if this node has more work to do, submit its current state vector, and then proceed to the next node
		data->submitToNNQueue(state_vector, node_num);		


	}

}



void threadManagerSimulation(string infile, int num_nodes, int minibatch_size, int num_threads) {

	// read in all the state vectors from the file
	vector<MCTS_Node*>* all_nodes = new vector<MCTS_Node*>(num_nodes);
	readStates(infile, all_nodes);

	// initialize shared state data objects
	MCTS_Thread_Manager *data = new MCTS_Thread_Manager(all_nodes, num_nodes, minibatch_size, num_threads);

	// register this thread for logging
	data->registerThreadName(this_thread::get_id(), "Master");

	// Spawn worker threads
	thread worker_threads[num_threads];
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		worker_threads[thread_num] = thread(workerFunc, thread_num, data);
	}

	// iterate over all active minibatches until there are none left
	// nextActiveMinibatchNum will return -1 when all minibatches are completed
	int minibatch_num;
	vector<int>* rounds_per_minibatch = new vector<int>(num_nodes / minibatch_size, 0);

	while ((minibatch_num = data->nextActiveMinibatchNum()) != -1) {

		// wait till it's safe to process this minibatch, and do an extra check that it's not complete
		data->masterWaitForMinibatch(minibatch_num);


		int round = rounds_per_minibatch->at(minibatch_num);

		string state_vector_file = "data/tmp/state_vectors_" + to_string(round) + "_" + to_string(minibatch_num);
		string action_distribution_file = "data/tmp/action_distributions_" + to_string(round) + "_" + to_string(minibatch_num);

		// read the NN queue for this minibatch and write it into a file
		data->serializeNNQueue(minibatch_num, state_vector_file);

		// fork a new process and exec the NN script in that process
		// the NN script will read from the state vectors file, and write to the action distributions file
		data->invokePythonScript("src/script.py", state_vector_file, action_distribution_file);

		// read in the resulting NN action distributions and write them into the NN output queue
		data->deserializeNNResults(minibatch_num, action_distribution_file, round);

		rounds_per_minibatch->at(minibatch_num)++;


	}

	// wait for all workers to join
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		worker_threads[thread_num].join();
	}

	// finally write all the nodes to file
	int num_minibatches = num_nodes / minibatch_size;
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		string node_file = "data/tmp/nodes_" + to_string(minibatch_num);
		data->writeNodesToFile(minibatch_num, node_file);
	}
}





void simulationStartup() {
	system("exec rm -r data/tmp/*");

}

void simulationVerification(int sim_num) {
	int result;
	result = system("exec diff data/tmp/nodes_0 data/exp_nodes_0");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 0" << endl;
		exit(1);
	} 



	result = system("exec diff data/tmp/nodes_1 data/exp_nodes_1");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 1" << endl;
		exit(1);
	}

	result = system("exec diff data/tmp/nodes_2 data/exp_nodes_2");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 2" << endl;
		exit(1);
	}

	result = system("exec diff data/tmp/nodes_3 data/exp_nodes_3");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 3" << endl;
		exit(1);
	}

	if (result == 0) {
		cout << "Simulation " << sim_num << " successful" << endl;
	} 
}

void sharedDataSimpleSimulation(int sim_num) {
	simulationStartup();
	int num_nodes = 64, minibatch_size = 16, num_threads = 4;
	int nodes_per_thread = 16, num_minibatches = 4;

	string initial_states_file = "data/initial_states.txt";

	threadManagerSimulation(initial_states_file, num_nodes, minibatch_size, num_threads);

	simulationVerification(sim_num);

}



void runThreadManagerTests() {
	cout << "Testing ThreadManager class..." << endl << endl;
	for (int sim_num = 0; sim_num < 100; sim_num++) {
		sharedDataSimpleSimulation(sim_num);
	}
	cout << endl << "Done testing ThreadManager class." << endl << endl;
}
