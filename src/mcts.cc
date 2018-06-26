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




using namespace std;

int MINIBATCH_SIZE = 16; // NN batch size
int TOTAL_NUM_NODES = 64;
int NUM_MINIBATCHES = TOTAL_NUM_NODES / MINIBATCH_SIZE;
int NUM_THREADS = 4;
int NODES_PER_THREAD = TOTAL_NUM_NODES / NUM_THREADS;

int master_minibatch; // which batch master is processing

mutex cout_mutex, nodes_pushed_mutex;

std::mutex m_mutex;
std::condition_variable m_condVar;


 int nodes_pushed = 0;
// initialize nodes and queue
vector<int> nodes(TOTAL_NUM_NODES);
vector<int> queue(TOTAL_NUM_NODES);
vector<bool> nn_ready(TOTAL_NUM_NODES);
vector<int> batch_counters(TOTAL_NUM_NODES / MINIBATCH_SIZE);
vector<int> batch_ready(TOTAL_NUM_NODES / MINIBATCH_SIZE);


map<__thread_id, string> thread_nums;

int num_batches_ready = 0;
mutex shared_state_lock;

// the minibatch_ready_cv condition variable is used by worker threads to signal the master
// (It is implemented by the minibatch_ready_mutex).
// the master sleeps until a worker notifies it that a mini-batch is ready to be shipped to the neural network
// the master will wake up, and do an extra check to make sure that the nn_ready flag
// for this latest minibatch is set to true (or the minibatch is entirely completed).
mutex minibatch_nn_ready_mutex;
condition_variable minibatch_nn_ready_cv;

int num_active_threads;

vector<bool> finished_minibatches(TOTAL_NUM_NODES);

vector<int> nodes_left_per_minibatch(MINIBATCH_SIZE);
vector<int> nodes_left_per_thread(NODES_PER_THREAD);











void threadSafePrint(string str) {
	cout_mutex.lock();
	cout << thread_nums[this_thread::get_id()] << " - " << str << endl;
	cout_mutex.unlock();
}

int updateCounter(int node_num, mutex& mtx) {
	mtx.lock();
	batch_counters[node_num / MINIBATCH_SIZE]++;
	int counter = batch_counters[node_num / MINIBATCH_SIZE];
	mtx.unlock();
	return counter;
}


void atomicDecrement(int* var, mutex* mtx) {
	mtx->lock();
	*var -= 1;
	mtx->unlock();
}

bool workerSafe(int minibatch_num) {
	return !nn_ready[minibatch_num];
}

void workerThreadFunc(int thread_num) {
	// process my thread
	for (int node_num = thread_num; node_num < TOTAL_NUM_NODES; node_num += 4) {

		int minibatch_num = node_num / MINIBATCH_SIZE;

		// make sure this node is marked as nn_ready
		if (!workerSafe(node_num / MINIBATCH_SIZE)) {
			continue;
		}

		// do some computation with the node (tree exploration)
		nodes[node_num] = node_num;
		/* MCTS_Node* progressed_node = computeMcts(nodes[node_num]);
		nodes[node_num] = progressed_node; */

		// if the computation for this node is permanently finished, decrement the counter of nodes left for this thread
		/* if (progressed_node->isFinished()) {
			decrementVariable(nodes_left_per_minibatch[minibatch_num], &shared_state_mutex);
			decrementVariable(nodes_left_per_thread[thread_num], &shared_state_mutex);
		}
		*/

		// eventually reach a stage where we need to submit to queue
		queue[node_num] = nodes[node_num];

		// update the counter of nodes submitted
		int ctr = updateCounter(node_num, nodes_pushed_mutex);
		//threadSafePrint("Updated the node #" + to_string(i) + ". Now the counter is " + to_string(ctr));
		
		// if the submission of this node means the minibatch is now ready, signal the master
		if (ctr == MINIBATCH_SIZE) {
			
			shared_state_lock.lock();
			threadSafePrint("Minibatch " + to_string(minibatch_num) + " complete");
			num_batches_ready += 1;

			// need to check if all dudes in this minibatch are complete, if so set finished_minibatches for it
			//finished_minibatches[batch_completed] = true;

			shared_state_lock.unlock();
			// wake up the master

   			minibatch_nn_ready_cv.notify_one();

		}

		
	}
	threadSafePrint("WORKER THREAD FINISHING: " + to_string(thread_num));
	shared_state_lock.lock(); // replace this with a "shared_state_lock, used for counters and flags"
	num_active_threads--;
	shared_state_lock.unlock();
	/*cout_mutex.lock();
	cout << "I am a worker thread! " << this_thread::get_id() << endl;
	cout_mutex.unlock();*/
}

//check that master_minibatch is nn_ready OR master_minibatch is fully completed
bool batchReady() {
	return num_batches_ready > 0 || finished_minibatches[master_minibatch] == true; //return batch_ready[batch_num];
}

MCTS_Node::MCTS_Node(int x) {
	this->x = x;
}


void processLine(string line, int state_num, vector<MCTS_Node*> &node_vec) {
	try {
		// this will be replaced by code to parse a full state vector line
    	node_vec[state_num] = new MCTS_Node(stoi(line));
  	}
  	catch (int e) {
    	cout << "Line from state-vector data file could not be parsed - exception #" << e << '\n';
  	}
}

// this whole function will be replaced by code to deserialize state vectors (protobufs)?
void readStates(string infile, vector<MCTS_Node*>& node_vec) {
	threadSafePrint("Reading state vectors from the file " + infile);

	string line;
	ifstream f (infile);
	int state_num = 0;
	while(getline(f, line)) {
    	processLine(line, state_num, node_vec);
    	state_num++;
    }
}

void callScript() {
	char *command = (char *) "python";
	char *scriptName= (char *) "src/script.py";  // it can also be resolved using your PATH environment variable
    char *pythonArgs[]={command, scriptName, NULL};

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

// when last worker thread dies, must signal master to check all batches
void runPythonScript(string filename) {

	// register this thread for logging purposes
	thread_nums[this_thread::get_id()] = "MASTER";

	// start here
	threadSafePrint("Beginning to read this batch.");

	// read in all the state vectors from the file
	vector<MCTS_Node*> all_nodes(TOTAL_NUM_NODES);
	readStates(filename, all_nodes);


	// Initialize all the nodes
	for (int i = 0; i < TOTAL_NUM_NODES; i++) {
		nodes[i] = i;
		queue[i] = 0;
		//nn_ready[i] = true;
		batch_ready[i / MINIBATCH_SIZE] = false;
	}

	// every minibatch has an associated flag called nn_ready
	// this flag is set to true if the minibatch is safe for a worker thread to process
	// the worker thread to finish processing nodes in this minibatch will set the flag to false
	// the master, before touching a minibatch, will make sure nn_ready is false
	// the master will ship the minibatch-batch to the NN, and once the results are back, will reset the flag to true
	// the nn_ready_lock mutex protects these per-minibatch flags
	// only one thread is allowed to modify the flags at any time.
	for (int minibatch_num = 0; minibatch_num < NUM_MINIBATCHES; minibatch_num++) {
		nn_ready[minibatch_num] = false;
		// this will be set by a worker thread when there is no more computation to be done on this minibatch
		finished_minibatches[minibatch_num] = false; 

		nodes_left_per_minibatch[minibatch_num] = MINIBATCH_SIZE;
	}
	mutex nn_ready_lock; // must be acquired before flipping any of the nn_ready flags


	// Spawn worker threads
	thread worker_threads[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {
		nodes_left_per_thread[i] = NODES_PER_THREAD;
		worker_threads[i] = thread(workerThreadFunc, i);
		thread_nums[worker_threads[i].get_id()] = "Worker " + to_string(i);
	}

	// this lock is required in order to implement the minibatch_nn_ready_cv.wait() functionality for the master thread
	// to do: understand why a lock is needed for cv.wait(), and what unique_lock does
	unique_lock<mutex> minibatch_nn_ready_lock(minibatch_nn_ready_mutex);
	
	// the master thread keeps spinning (wait for mini-batch, ship to NN, receive and publish results) as long as there is some active thread
	num_active_threads = NUM_THREADS;



	do {
		for (master_minibatch = 0; master_minibatch < NUM_MINIBATCHES; master_minibatch++) {
			
			// The master will sleep until one of the worker threads signals that a minibatch is ready for shipment to the neural network\
			// It will wake up when signaled, and then check that one of the following cases is true:
			// 1. the mini-batch indexed by master_minibatch is ready for NN-shipment
			// 2. That minibatch has completed, in which case we just increment master_minibatch, and sleep
			minibatch_nn_ready_cv.wait(minibatch_nn_ready_lock, batchReady);

			// if the minibatch is entirely completed, just move on
			if (finished_minibatches[master_minibatch]) {
				continue;
			}


	    	threadSafePrint("MASTER AWAKENED, Ready to process batch " + to_string(master_minibatch));    
			
	    	// read from the queue the current minibatch of state vectors, and write them to a file
	    	// ship those state vectors of to the Tensorflow script,
	    	// which will unparse them, pipe them through the NN, and write the recommended action distributions into a file.
	    	// Then read those action distributions from a file, unparse them, and place them in the global nn_outputs vector for the workers to access. 
			callScript();



	    	// decrement number of ready batches
	    	shared_state_lock.lock();
	    	num_batches_ready -= 1;
	    	shared_state_lock.unlock();

	    	threadSafePrint("FINSIHED CALLING SCRIPT FOR THE " + to_string(master_minibatch) + "th time. Master sleeping now");


	 	}
	} while (num_active_threads > 0);

    // wait for workers to join
	for (int i = 0; i < 4; i++) {
		worker_threads[i].join();
	}

	threadSafePrint("All worker threads joined");
	
}