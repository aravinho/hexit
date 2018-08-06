#include "main.h"
#include "utils.h"
#include "config.h"

#include <iostream>
#include "stdlib.h"
#include <ctime>
#include <chrono>

using namespace std;


// parses an array of arguments (typically from command line) into an ArgMap.
void parseArgs(int argc, char* argv[], ArgMap* arg_map, int start_index) {


	ASSERT(argc >= 0 && (argc - start_index) % 2 == 0, "There must be an even number of args from start_index till the end");
	ASSERT(argv != NULL && arg_map != NULL, "Argv or Str_args is NULL");
	ASSERT(start_index >= 0 && start_index < argc, "Invalid start_index in main parseArgs");

	string option, value;

	int num_pairs = (argc - start_index) / 2;
	for (int pair_num = 0; pair_num < num_pairs; pair_num += 1) {
		option = string(argv[(pair_num * 2) + start_index]);
		ASSERT(option.size() > 2 && option.substr(0, 2) == "--", "Arg options must start with double hyphen");
		option = option.substr(2);
		value = string(argv[(pair_num * 2) + 1 + start_index]);
		arg_map->insert(option, value);

	}

}

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




