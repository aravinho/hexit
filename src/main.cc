#include "main.h"
#include "utils.h"
#include "config.h"

#include <iostream>
#include "stdlib.h"
#include <ctime>
#include <chrono>

using namespace std;







/*void nMCTSWorkerFunc(int thread_num, MCTS_Thread_Manager* data, int max_depth) {

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

*/


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





int main(int argc, char *argv[]) {

	cout << endl << endl;

	// parse all command line arguments into an ArgMap instance
	ArgMap* arg_map = new ArgMap();
	parseArgs(argc, argv, arg_map);
	cout << endl << endl;

	string game = arg_map->getString("game", DEFAULT_GAME);
	// make sure game is valid
	ASSERT(GAME_OPTIONS.count(game) > 0, "Given game " << game << " not available");
	cout << "Game: " << game << endl;

	// grab the number of states
	int num_states = arg_map->getInt("num_states", DEFAULT_NUM_STATES);
	cout << "Running MCTS from " << num_states << " states." << endl;

	// grab the input data path where the states are stored in CSV files
	string input_data_path = arg_map->getString("input_data_path");
	cout << "input data path: " << input_data_path << endl;

	/*// game options/specs
	if (game == "hex") {
		int hex_dim = (arg_options->count("hex_dim") > 0) ? stoi(arg_options->at("hex_dim")) : DEFAULT_HEX_DIM;
		cout << "hex_dim: " << hex_dim << endl;
		string hex_reward_type = (arg_options->count("hex_reward_type") > 0) ? (arg_options->at("hex_reward_type")) : DEFAULT_HEX_REWARD_TYPE;
		cout << "hex_reward_type: " << hex_reward_type << endl;
	}

	

	// num simulations per state
	int num_simulations = (arg_options->count("num_simulations") > 0) ? stoi(arg_options->at("num_simulations")) : DEFAULT_NUM_SIMULATIONS;
	ASSERT(num_simulations >= 0, "Cannot have a negative number of simulations");
	cout << "num simulations: " << num_simulations << endl;

	// max depth per simulation
	int max_depth = (arg_options->count("max_depth") > 0) ? stoi(arg_options->at("max_depth")) : DEFAUlT_MAX_DEPTH;
	ASSERT(max_depth >= 0, "Cannot have a negative max_depth");
	cout << "max_depth: " << max_depth << endl;

	// minibatch size
	int minibatch_size = (arg_options->count("minibatch_size") > 0) ? stoi(arg_options->at("minibatch_size")) : DEFAULT_MINIBATCH_SIZE;
	ASSERT(minibatch_size >= 0, "Cannot have a negative minibatch size");
	cout << "minibatch_size: " << minibatch_size << endl;

	// num_threads
	int num_threads = (arg_options->count("num_threads") > 0) ? stoi(arg_options->at("num_threads")) : DEFAULT_NUM_THREADS;
	ASSERT(num_threads >= 0, "Cannot have a negative num_threads");
	cout << "num_threads: " << num_threads << endl;

	// log_every
	int log_every = (arg_options->count("log_every") > 0) ? stoi(arg_options->at("log_every")) : DEFAULT_LOG_EVERY;
	ASSERT(log_every >= 0, "Cannot have a negative log_every");
	cout << "log_every: " << log_every << endl;

	// whether to use a NN apprentice
	bool use_nn = false;
	if (arg_options->count("use_nn") > 0 && arg_options->at("use_nn") == "True") {
		use_nn = true;
	}

	// NN options
	if (use_nn) {

		// model spec
		ASSERT(arg_options->count("model_spec") > 0, "If you want to use NN, must specify a model_spec");
		string model_spec = arg_options->at("model_spec");
		cout << "model spec: " << model_spec << endl;

		// model path
		ASSERT(arg_options->count("model_path") > 0, "If you want to use NN, must specify a model_path");
		string model_path = arg_options->at("model_path");
		cout << "model path: " << model_path << endl;

		// nn querying script
		ASSERT(arg_options->count("nn_script") > 0, "If you want to use NN, must specify a nn_script");
		string nn_script = arg_options->at("nn_script");
		cout << "nn_script: " << nn_script << endl;
	}

	// write final data to file?
	bool save_data = (arg_options->count("output_data_path") > 0);
	if (save_data) {
		string output_data_path = arg_options->at("output_data_path");
		int states_per_file = (arg_options->count("states_per_file") > 0) ? stoi(arg_options->at("states_per_file")) : DEFAULT_STATES_PER_FILE;
		cout << "output_data_path: " << output_data_path << endl;
		cout << "states_per_file: " << states_per_file << endl;
	}

*/

	// read in input data
	vector<MCTS_Node*>* nodes = new vector<MCTS_Node*>();
	readInputData(game, num_states, input_data_path, nodes, *arg_map);

	// determine whether to use an NN apprentice
	bool requires_nn = arg_map->getBool("requires_nn", DEFAULT_REQUIRES_NN);

	// if no NN is required, run standard MCTS
	if (!requires_nn) {
		int max_depth = arg_map->getInt("max_depth", DEFAULT_MAX_DEPTH);
		int log_every = arg_map->getInt("log_every", DEFAULT_LOG_EVERY);

		for (int node_num = 0; node_num < nodes->size(); node_num++) {
			if (node_num % log_every == 0) {
				cout << "node_num: " << node_num << endl;
			}
			MCTS_Node* node = nodes->at(node_num);
			runAllSimulations(node, max_depth);
			
		}
	}


	// write final states and action distributions to file
	bool save_data = arg_map->contains("output_data_path");
	if (save_data) {
		string output_data_path = arg_map->getString("output_data_path");
		int states_per_file = arg_map->getInt("states_per_file", DEFAULT_STATES_PER_FILE);
		int num_nodes_written = writeBatchToFile(nodes, output_data_path, states_per_file);
	}





	cout << endl << endl;

	exit(0);
	/*
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

	cout << "Finished at " << ctime(&end_time) << endl;*/
}




