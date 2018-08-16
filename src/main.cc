#include "main.h"
#include "utils.h"
#include "config.h"
#include "mcts_thread_manager.h"
#include "inference.h"

#include <iostream>
#include "stdlib.h"
#include <ctime>
#include <chrono>
#include <thread>

using namespace std;



// worker thread function to run vanilla MCTS (with no NN apprentice)
void threadFunc(int thread_num, vector<MCTS_Node*>* nodes, int start, int end, int max_depth, int log_every) {

	ASSERT(nodes != NULL, "Cannot pass a null nodes array to threadFunc");
	ASSERT(start >= 0 && end <= nodes->size() && start <= end, "Start and end index must be in range, and end index cannot be smaller than start index");
	ASSERT(log_every > 0, "log_every must be positive");

	for (int node_num = start; node_num < end; node_num++) {
		if ((node_num - start) % log_every == 0) {
			logTime("Thread " + to_string(thread_num) + ": Processing state #" + to_string(node_num));
		}
		MCTS_Node* node = nodes->at(node_num);
		runAllSimulations(node, max_depth);

	}

}

void nMCTSThreadFunc(int thread_num, vector<MCTS_Node*>* nodes, int start, int end, MCTS_Thread_Manager* thread_manager, const ArgMap& arg_map) {


	// unpack args
	int batch_size = arg_map.getInt("minibatch_size", DEFAULT_MINIBATCH_SIZE);
	int max_depth = arg_map.getInt("max_depth", DEFAULT_MAX_DEPTH);
	int log_every = arg_map.getInt("log_every", DEFAULT_LOG_EVERY);
	string model_path = arg_map.getString("model_path");
	
	ASSERT(nodes != NULL, "Cannot pass a null nodes array to threadFunc");
	ASSERT(start >= 0 && end <= nodes->size() && start <= end, "Start and end index must be in range, and end index cannot be smaller than start index");
	ASSERT(log_every > 0, "log_every must be positive");
	ASSERT(max_depth > 0 && batch_size > 0, "max depth and batch_size must be positive");

	// register thread and log
	thread_manager->registerThreadName(this_thread::get_id(), "Worker " + to_string(thread_num));
	thread_manager->log("N-MCTS Thread " + to_string(thread_num) + " working on nodes " + to_string(start) + " to " + to_string(end)
	+ " with max depth " + to_string(max_depth) + " and batch_size " + to_string(batch_size));


	// determine number of batches, and size of action distributiosn
	int num_states = end - start;
	int num_batches = num_states / batch_size;
	if (num_batches == 0) {
		num_batches = 1;
	}
	int num_actions = nodes->at(start)->getState()->numActions();

	// start TF session
	const string meta_graph_path = model_path + ".meta";
    const string model_checkpoint_path = model_path;

	// initialize the TF session
    Session* session = NewSession(SessionOptions());
    ASSERT(session != NULL, "Session is NULL");
    // Read in the protobuf graph we exported, and add it to the session
    MetaGraphDef graph_def;
    Status status = restoreModelGraph(session, &graph_def, meta_graph_path, model_checkpoint_path);
    ASSERT(status.ok(), "Error restoring model graph");
    thread_manager->log("Successfully loaded metagraph and all nodes from model checkpoint at " + model_checkpoint_path);


    int num_states_finished = 0;

	for (int batch_num = 0; batch_num < num_batches; batch_num++) {
		int num_unfinished_trees = batch_size;

		// initialize vector of ActionDistributions
		vector<ActionDistribution*> action_dists(batch_size, NULL);
		// initialize bitvector of whether trees are complete
		vector<bool> finished_tree(batch_size, false);
		// initialize vector of EnvStates, which will be used for inference
		vector<EnvState*> states(batch_size);
		for (int batch_index = 0; batch_index < batch_size; batch_index++) {
			int state_num = start + (batch_num * batch_size) + batch_index;
			states[batch_index] = nodes->at(state_num)->getState();

		}



		while (num_unfinished_trees > 0) {
			
		
			// run each state until it needs inference
			for (int batch_index = 0; batch_index < batch_size; batch_index++) {
				int state_num = start + (batch_num * batch_size) + batch_index;
				if (finished_tree[batch_index]) {
					continue;
				}


				
				// run MCTS on this tree until either all simulations are done, or it requires an NN prediction
				MCTS_Node* node = nodes->at(state_num);
				node = runMCTS(node, max_depth, action_dists[batch_index]);
				nodes->at(state_num) = node;

				// if the node is now finished
				if (node->simulationsFinished()) {
					finished_tree[batch_index] = true;
					num_unfinished_trees -= 1;
					num_states_finished += 1;
					if (num_states_finished % log_every == 0) {
						thread_manager->log("Finished " + to_string(num_states_finished) + " out of " + to_string(num_states) + " states");
					}
				}

				// if node still active, submit state vector the NN batch
					//thread_manager->log("Submitting state " + to_string(state_num) + " to state batch");
				states[batch_index] = node->getState();
			
			}

			// create a TF feed dict from this batch, and run inference
			vector<pair<string, Tensor>> feed_dict;
			vector<string> output_ops;
			createTensorsFromStates(batch_size, states, &feed_dict, &output_ops);
			vector<Tensor> output_tensors;
			
			Status status = predictBatch(session, feed_dict, output_ops, &output_tensors);



			// unpack the actions for each of the states, and create ActionDistribution objects
			for (int batch_index = 0; batch_index < batch_size; batch_index++) {
				
				// if tree is already finished, don't bother
				if (finished_tree[batch_index]) {
					continue;
				}

				int state_num = start + (batch_num * batch_size) + batch_index;
				
				// unpack the action distribution predicted by NN inference
				Tensor action_dist_tensor = output_tensors[0];
				auto action_dist = action_dist_tensor.matrix<float>();
				vector<double>* ad_vec = new vector<double>(num_actions);
				for (int action_num = 0; action_num < num_actions; action_num++) {
					ad_vec->at(action_num) = action_dist(batch_index, action_num);
				}

				// create an ActionDistribution object, and place it in the action_dists vector.
				ActionDistribution* new_ad = new ActionDistribution(ad_vec);
				ASSERT(ad_vec != NULL, "Just created an action distribution with a null advec");
				action_dists[batch_index] = new_ad;
				//thread_manager->log("Created action distribution for state " + to_string(state_num));

			}

		}
	}

}











int main(int argc, char *argv[]) {

	cout << endl << endl;

	// parse all command line arguments into an ArgMap instance
	ArgMap* arg_map = new ArgMap();
	parseArgs(argc, argv, arg_map);

	// unpack a few key arguments
	string game = arg_map->getString("game", DEFAULT_GAME);
	ASSERT(GAME_OPTIONS.count(game) > 0, "Given game " << game << " not available");
	int num_states = arg_map->getInt("num_states", DEFAULT_NUM_STATES);
	string input_data_path = arg_map->getString("input_data_path");
	int start_at = arg_map->getInt("start_at", DEFAULT_START_AT);

	// read in input data
	vector<MCTS_Node*>* nodes = new vector<MCTS_Node*>();
	readInputData(game, num_states, input_data_path, start_at, nodes, *arg_map);

	// determine whether to use an NN apprentice
	bool requires_nn = arg_map->getBool("requires_nn", DEFAULT_REQUIRES_NN);

	// if no NN is required, run standard MCTS
	if (!requires_nn) {

		int max_depth = arg_map->getInt("max_depth", DEFAULT_MAX_DEPTH);
		int log_every = arg_map->getInt("log_every", DEFAULT_LOG_EVERY);

		logTime("Beginning MCTS on " + to_string(num_states) + " states");

		// divide the nodes array into 4 segments, and pass each segment to an individual thread
		int num_threads = arg_map->getInt("num_threads", DEFAULT_NUM_THREADS);
		int states_per_thread = num_states / num_threads;
		thread worker_threads[num_threads];
		for (int thread_num = 0; thread_num < num_threads; thread_num++) {
			int start = thread_num * states_per_thread;
			int end = (thread_num + 1) * states_per_thread;
			worker_threads[thread_num] = thread(threadFunc, thread_num, nodes, start, end, max_depth, log_every);
		}

		// wait for all worker threads to join
		for (int thread_num = 0; thread_num < num_threads; thread_num++) {
			worker_threads[thread_num].join();
		}

		logTime("Done with MCTS on " + to_string(num_states) + " states");

	}

	else {
		cout << "REQUIRES NN" << endl;
		int max_depth = arg_map->getInt("max_depth", DEFAULT_MAX_DEPTH);
		int log_every = arg_map->getInt("log_every", DEFAULT_LOG_EVERY);
		int minibatch_size = arg_map->getInt("minibatch_size", DEFAULT_MINIBATCH_SIZE);

		logTime("Beginning N-MCTS on " + to_string(num_states) + " states");

		MCTS_Thread_Manager thread_manager(nodes);

		// divide the nodes array into 4 segments, and pass each segment to an individual thread
		int num_threads = arg_map->getInt("num_threads", DEFAULT_NUM_THREADS);
		int states_per_thread = num_states / num_threads;
		thread worker_threads[num_threads];
		for (int thread_num = 0; thread_num < num_threads; thread_num++) {
			int start = thread_num * states_per_thread;
			int end = (thread_num + 1) * states_per_thread;
			worker_threads[thread_num] = thread(nMCTSThreadFunc, thread_num, nodes, start, end, &thread_manager, *arg_map);
			//worker_threads[thread_num] = thread(nMCTSThreadFunc, thread_num, nodes, start, end, &thread_manager, *arg_map);
		}

		// wait for all worker threads to join
		for (int thread_num = 0; thread_num < num_threads; thread_num++) {
			worker_threads[thread_num].join();
		}

		logTime("Done with MCTS on " + to_string(num_states) + " states");

		MCTS_Node* node = nodes->at(0);
		cout << "node: " << endl;
		node->getState()->printBoard();
		vector<int>* ac = node->getActionCounts();
		printVector(*ac, "action counts");
	}

	// write final states and action distributions to file
	bool save_data = arg_map->contains("output_data_path");
	if (save_data) {
		string output_data_path = arg_map->getString("output_data_path");
		int states_per_file = arg_map->getInt("states_per_file", DEFAULT_STATES_PER_FILE);
		int num_nodes_written = writeBatchToFile(nodes, output_data_path, states_per_file);
	}

	cout << endl << endl;

	return 0;

}




