#include "mcts.h"
#include "mcts_shared_data.h"

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





// TO DO: find why markNode
void workerFunc(int thread_num, MCTS_Shared_Data* data) {

	// Register this thread for logging
	data->registerThreadName(this_thread::get_id(), "Worker " + to_string(thread_num));


	// iterate through all the active nodes in my thread
	// nextActiveNodeNum will return -1 when there are no more nodes for me to process
	int node_num;
	while ((node_num = data->nextActiveNodeNum(thread_num)) != -1) {
		// wait till it's safe to touch the minibatch that holds this node
		int minibatch_num = data->getMinibatchNum(node_num);
		data->workerWaitForNode(node_num, thread_num);

		// grab the node, and its corresponding NN output (Action distribution) that the master has placed
		MCTS_Node* node = data->getNode(node_num);
		ActionDistribution* ad = data->getNNOutput(node_num);

		// check stuff
		if (ad->z != 0 && ad->z != node->x) {
			data->log("YELL, node " + to_string(node->init_x) + " is messed. ad->Z = " + to_string(ad->z) + ",  node->x = " + to_string(node->x));
		}
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
		//data->log("foo");

		// if this node has more work to do, submit its current state vector, and then proceed to the next node
		data->submitToNNQueue(state_vector, node_num);		


	}

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


void callScript(string infile, string outfile) {
	char *command = (char *) "python";
	char *scriptName= (char *) "src/script.py";  // it can also be resolved using your PATH environment variable
	char infile_arg[100]; strcpy(infile_arg, infile.c_str());
	char outfile_arg[100]; strcpy(outfile_arg, outfile.c_str());
    char *pythonArgs[]={command, scriptName, infile_arg, outfile_arg, NULL};

    pid_t parent = getpid();
    pid_t pid = fork();

    // if child process, do the exec
    if (pid == 0) {
    	execvp(command, pythonArgs);
     }

    // for the parent process, wait till the child completes
    else {
        waitpid(pid, NULL, 0);
    }
}

void masterFunc(string infile, int num_nodes, int minibatch_size, int num_threads) {

	// read in all the state vectors from the file
	vector<MCTS_Node*>* all_nodes = new vector<MCTS_Node*>(num_nodes);
	readStates(infile, all_nodes);

	// initialize shared state data objects
	MCTS_Shared_Data *data = new MCTS_Shared_Data(all_nodes, num_nodes, minibatch_size, num_threads);

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
		//data->log("Master starting on minibatch " + to_string(minibatch_num) + " in round " + to_string(round));


		string state_vector_file = "data/tmp/state_vectors_" + to_string(round) + "_" + to_string(minibatch_num);
		string action_distribution_file = "data/tmp/action_distributions_" + to_string(round) + "_" + to_string(minibatch_num);

		// read the NN queue for this minibatch and write it into a file
		data->writeMinibatchToFile(minibatch_num, state_vector_file);

		// fork a new process and exec the NN script in that process
		// the NN script will read from the state vectors file, and write to the action distributions file
		callScript(state_vector_file, action_distribution_file);

		// quick check to make sure got copied correctly
		string call = "exec diff " + state_vector_file + " " + action_distribution_file;
		int result = system(call.c_str());
		if (result != 0) {
			cout << "Error in copying files. Round: " << round << ", minibatch num: " << minibatch_num << endl;
			
		}

		// read in the resulting NN action distributions and write them into the NN output queue
		data->unparseNNResults(minibatch_num, action_distribution_file, round);

		data->log("Master finished with minibatch " + to_string(minibatch_num) + " in round " + to_string(round));


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
		data->writeMinibatchNodesToFile(minibatch_num, node_file);
	}
}





