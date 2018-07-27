#include "main.h"
#include <iostream>
#include "stdlib.h"
#include <ctime>
#include <chrono>
using namespace std;

int NODES_PER_BATCH = pow(2,16);




vector<MCTS_Node*>* generateRandomNodes(int num_states, int num_simulations=1000) {
	
	Tictactoe* board;
	MCTS_Node* node;

	vector<MCTS_Node*>* nodes = new vector<MCTS_Node*>(num_states);
	for (int state_num = 0; state_num < num_states; state_num++) {
		board = generateRandomTictactoeBoard();
		node = (new MCTS_Node(board, true, num_simulations))->sampleActions(true);
		nodes->at(state_num) = node;
	}

	return nodes;
}


void nMCTSWorkerFunc(int thread_num, MCTS_Thread_Manager* data, int max_depth) {

	// Register this thread for logging
	data->registerThreadName(this_thread::get_id(), "Worker " + to_string(thread_num));
	data->log("registered");


	// iterate through all the active nodes in my thread
	// nextActiveNodeNum will return -1 when there are no more nodes for me to process
	int node_num;
	while ((node_num = data->nextActiveNodeNum(thread_num)) != -1) {
		// wait till it's safe to touch the minibatch that holds this node
		data->workerWaitForNode(node_num, thread_num);
		bool suppress_log = data->getMinibatchNum(node_num) != 0;
		suppress_log = true;
		bool log_mcts = !suppress_log;
		bool force = false;

		// grab the node, and its corresponding NN output (Action distribution) that the master has placed
		MCTS_Node* node = data->getNode(node_num);
		ActionDistribution* ad = data->getNNResult(node_num);
		data->log("Worker grabbed node and ad for node " + to_string(node_num) + " ", force, suppress_log);
	
		// process the node till it must wait, and grab the state vector it submits
		// if the node is complete to begin with, this call will do nothing
		MCTS_Node* new_node = runMCTS(node, ad, max_depth, log_mcts);
		StateVector* state_vector = new_node->getStateVector();
		data->setNode(node_num, new_node); // need to implement



		// mark that this thread has processed this node, so it doesn't loop around and do so again until the master has refreshed its minibatch.
		
		data->markNodeProcessed(node_num, thread_num);
		
		// if this node was already complete, or is now complete, mark it complete
		// there is no need to submit any state vector
		if (new_node->simulationsFinished()) {
			data->markNodeComplete(node_num);
			data->log("Node Permanently Finished: " + to_string(node_num));
			continue;
		}

		// if this node has more work to do, submit its current state vector, and then proceed to the next node
		data->submitToNNQueue(state_vector, node_num);		
		data->log("Worker finished submitting node " + to_string(node_num) + " ", force, suppress_log);


	}

}




void generateRandomDataBatchNMCTS(int num_nodes, int num_actions, string model_dirname, string data_base_dirname, string script_filename, int minibatch_size, int num_threads, int num_simulations, int max_depth, int states_per_file, int log_every) {

	// create all the nodes from the file
	vector<MCTS_Node*>* all_nodes = generateRandomNodes(num_nodes, num_simulations);

	// initialize shared state data objects
	MCTS_Thread_Manager *data = new MCTS_Thread_Manager(all_nodes, num_nodes, minibatch_size, num_threads, num_actions);



	// register this thread for logging
	data->registerThreadName(this_thread::get_id(), "Master");
	data->log("Registered");

	// create a temp directory where the workers and masters will communicate through files
	string tmp_sv_dirname = createDir(data_base_dirname + "tmp/sv/");
	string tmp_ad_dirname = createDir(data_base_dirname + "tmp/ad/");
	// Spawn worker threads
	thread worker_threads[num_threads];
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		worker_threads[thread_num] = thread(nMCTSWorkerFunc, thread_num, data, max_depth);
	}


	// iterate over all active minibatches until there are none left
	// nextActiveMinibatchNum will return -1 when all minibatches are completed
	int minibatch_num;
	vector<int>* rounds_per_minibatch = new vector<int>(num_nodes / minibatch_size, 0);

	while ((minibatch_num = data->nextActiveMinibatchNum()) != -1) {

		// wait till it's safe to process this minibatch, and do an extra check that it's not complete
		data->masterWaitForMinibatch(minibatch_num);

		int round = rounds_per_minibatch->at(minibatch_num);


		string state_vector_file = tmp_sv_dirname + to_string(round) + "_" + to_string(minibatch_num) + ".csv";
		string action_distribution_file = tmp_ad_dirname + to_string(round) + "_" + to_string(minibatch_num) + ".csv";

		// read the NN queue for this minibatch and write it into a file
		data->serializeNNQueue(minibatch_num, state_vector_file);

		// fork a new process and exec the NN script in that process
		// the NN script will read from the state vectors file, and write to the action distributions file
		data->invokeNNScript(script_filename, state_vector_file, model_dirname, action_distribution_file);


		// read in the resulting NN action distributions and write them into the NN output queue
		data->deserializeNNResults(minibatch_num, action_distribution_file, round);

		rounds_per_minibatch->at(minibatch_num)++;

		int active_nodes_left = data->numActiveNodesInMinibatch(minibatch_num);


	}

	// wait for all workers to join
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		worker_threads[thread_num].join();
	}

	// remove the temp directories (TO DO)

	// finally write all the nodes to file
	all_nodes = data->getAllNodes();
	writeBatchToFile(all_nodes, data_base_dirname, states_per_file);



}






int main(int argc, char *argv[]) {
	
	chrono::system_clock::time_point start_time_point = chrono::system_clock::now();
	time_t start_time = chrono::system_clock::to_time_t(start_time_point);
	cout << "Beginning at " << ctime(&start_time) << endl;

	srand(time(NULL));

	int num_states = pow(2,14);
	int num_actions = 9;
	int num_simulations = 1000;
	int max_depth = 4;
	int states_per_file = pow(2,20);
	int log_every = pow(2,9);
	string model_dirname = "models/tictactoe/test_batch/";
	int minibatch_size = pow(2, 6);
	int num_threads = 4;
	string script_name = "src/nn_query.py";
	string base_dirname;


	assert (argc == 2);
	string option = string(argv[1]);

	if (option == "--rawmcts") {
		int num_rounds = 4;
		base_dirname = "data/in/train/tictactoe/raw_mcts/";
		for (int round = 0; round < num_rounds; round++) {
			generateRandomDataBatch(num_states, base_dirname, num_simulations, max_depth, states_per_file, log_every);
		}
	}

	if (option == "--nmcts") {
		base_dirname = "data/in/train/tictactoe/n_mcts/";
		generateRandomDataBatchNMCTS(num_states, num_actions, model_dirname, base_dirname, script_name, minibatch_size, num_threads, num_simulations, max_depth, states_per_file, log_every);
	}
	
	

	chrono::system_clock::time_point end_time_point = chrono::system_clock::now();
	time_t end_time = chrono::system_clock::to_time_t(end_time_point);
	chrono::system_clock::duration time_elapsed = start_time_point - end_time_point;

	cout << "Finished at " << ctime(&end_time) << endl;
}



