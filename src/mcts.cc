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




using namespace std;

MCTS_Shared_Data::MCTS_Shared_Data(vector<MCTS_Node*>* all_nodes, int num_nodes, int minibatch_size, int num_threads) {
	if (num_nodes % minibatch_size != 0) {
		throw invalid_argument("The minibatch size must be divisible by the total number of nodes");
	}
	if (num_nodes % num_threads != 0) {
		throw invalid_argument("The number of worker threads must be divisible by the total number of nodes");
	}

	this->num_nodes = num_nodes;
	this->minibatch_size = minibatch_size;
	this->num_threads = num_threads;
	this->num_minibatches = num_nodes / minibatch_size;
	this->num_nodes_per_thread = num_nodes / num_threads;

	this->all_nodes = all_nodes;
	this->nn_queue = new vector<StateVector*>(num_nodes);
	this->nn_outputs = new vector<ActionDistribution*>(num_nodes);

	this->minibatch_ownership = new vector<bool>(num_minibatches);
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		this->minibatch_ownership->at(minibatch_num) = false;
	}

	this->nodes_left_to_submit = new vector<int>(num_minibatches);
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		this->nodes_left_to_submit->at(minibatch_num) = minibatch_size;
	}


	this->active_nodes_in_minibatch = new vector<int>(num_minibatches);
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		this->active_nodes_in_minibatch->at(minibatch_num) = minibatch_size;
	}

	this->active_nodes_in_thread = new vector<int>(num_threads);
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		this->active_nodes_in_thread->at(thread_num) = num_nodes_per_thread;
	}

	num_active_minibatches = num_minibatches;
	num_active_threads = num_threads;

	thread_names = new map<__thread_id, string>();

	cout << "Size of nn queue: " << nn_queue->size() << endl;

}

bool MCTS_Shared_Data::isWorkerSafe(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("isWorkerSafe: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	bool ret = !(this->minibatch_ownership->at(minibatch_num));
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Shared_Data::isMasterSafe(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("isMasterSafe: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	bool ret = this->minibatch_ownership->at(minibatch_num);
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Shared_Data::workerWaitForMinibatch(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("workerWaitForMinibatch: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isWorkerSafe(minibatch_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}

	return isMinibatchComplete(minibatch_num);
}

bool MCTS_Shared_Data::masterWaitForMinibatch(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("masterWaitForMinibatch: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isMasterSafe(minibatch_num) && !isMinibatchComplete(minibatch_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}

	return isMinibatchComplete(minibatch_num);
}

bool MCTS_Shared_Data::isMinibatchComplete(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("isMinibatchComplete: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	bool ret = active_nodes_in_minibatch->at(minibatch_num) == 0;
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Shared_Data::isThreadComplete(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("isThreadComplete: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	bool ret = active_nodes_in_thread->at(minibatch_num) == 0;
	shared_data_mutex.unlock();
	return ret;
}


MCTS_Node* MCTS_Shared_Data::getNode(int node_num) {
	if (node_num >= this->num_nodes) {
		throw invalid_argument("getNode: Cannot access the node numbered " + to_string(node_num));
	}
	return all_nodes->at(node_num);
}

void MCTS_Shared_Data::submitToNNQueue(StateVector* state_vector, int node_num) {
	if (node_num >= this->num_nodes) {
		throw invalid_argument("submitToNNQueue: Cannot submit the node numbered " + to_string(node_num));
	}
	if (state_vector == NULL) {
		throw invalid_argument("submitToNNQueue: Cannot submit a NULL state vector for node " + to_string(node_num));
	}

	int minibatch_num = getMinibatchNum(node_num);

	// wait till it's safe to touch this minibatch (it should be already), then submit the state vector
	workerWaitForMinibatch(minibatch_num);
	nn_queue->at(node_num) = state_vector;
	
	// decrement nodes_left_to_submit for this batch
	// if last node from this batch to submit, mark batch as master safe, and reset counter
	int num_nodes_left = decrementNodesLeft(minibatch_num);

	
}

void MCTS_Shared_Data::submitToNNOutputs(vector<ActionDistribution*>* nn_results, int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("submitToNNOutputs: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	if (nn_results == NULL || nn_results->size() != minibatch_size) {
		throw invalid_argument("submitToNNOutputs: Cannot submit NN Results that are null or the wrong size for minibatch " + to_string(minibatch_num));
	}

	// wait for it's safe to touch this minibatch (it should be already), then submit NN results for every node in this minibatch
	masterWaitForMinibatch(minibatch_num);
	for (int node_num = minibatch_num * minibatch_size; node_num < (minibatch_num + 1) * minibatch_size; node_num++) {
		this->nn_outputs->at(node_num) = nn_results->at(node_num);
	}

	// mark this minibatch worker safe
	shared_data_mutex.lock();
	markWorkerSafe(minibatch_num);
	shared_data_mutex.unlock();

	// notify any (and all) sleeping workers that there is a fresh minibatch to work on
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex); // do we need to unique_lock?
	minibatch_safe_cv.notify_all();

}

void MCTS_Shared_Data::completeNode(int node_num) {
	if (node_num >= this->num_nodes) {
		throw invalid_argument("completeNode : Cannot access the node numbered " + to_string(node_num));
	}

	int minibatch_num = getMinibatchNum(node_num);
	int thread_num = getThreadNum(node_num);

	// decrement the number of active nodes in this thread and minibatch
	shared_data_mutex.lock();
	active_nodes_in_minibatch->at(minibatch_num) -= 1;
	if (isMinibatchComplete(minibatch_num)) {
		num_active_minibatches--;
	}
	active_nodes_in_thread->at(thread_num) -= 1;
	if (isThreadComplete(minibatch_num)) {
		num_active_threads--;
	}
	shared_data_mutex.unlock();

	// decrement the number of nodes left to submit for this batch
	decrementNodesLeft(minibatch_num);
}

void MCTS_Shared_Data::markMasterSafe(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("markMasterSafe: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	minibatch_ownership->at(minibatch_num) = true;
	shared_data_mutex.unlock();
}

void MCTS_Shared_Data::markWorkerSafe(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("markWorkerSafe: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}
	shared_data_mutex.lock();
	minibatch_ownership->at(minibatch_num) = false;
	shared_data_mutex.unlock();
}

int MCTS_Shared_Data::decrementNodesLeft(int minibatch_num) {
	if (minibatch_num >= this->num_minibatches) {
		throw invalid_argument("decrementNodesLeft: Cannot access the minibatch numbered " + to_string(minibatch_num));
	}

		
	shared_data_mutex.lock();

	// grab the new number of nodes left to submit in this minibatch
	int num_nodes_left = nodes_left_to_submit->at(minibatch_num) - 1;

	// make sure there's no error
	if (num_nodes_left < 0) {
		throw logic_error("There are now fewer than 0 nodes left to submit for the minibatch numbered " + to_string(minibatch_num));
	}
	
	// if this is the last node from this batch to be submitted, mark batch as master safe, reset counter, and signal master
	if (num_nodes_left == 0) {
		num_nodes_left = active_nodes_in_minibatch->at(minibatch_num);
		markMasterSafe(minibatch_num);

		// notify the master that this minibatch is safe for it to process
		unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex); // do we need to unique_lock?
		minibatch_safe_cv.notify_one();

	}

	// actually perform the update to the counter
	nodes_left_to_submit->at(minibatch_num) = num_nodes_left;
	shared_data_mutex.unlock();


	return num_nodes_left;

}

int MCTS_Shared_Data::getMinibatchNum(int node_num) {
	if (node_num >= this->num_nodes) {
		throw invalid_argument("getMinibatchNum: Cannot submit the node numbered " + to_string(node_num));
	}
	return node_num / minibatch_size;
}

int MCTS_Shared_Data::getThreadNum(int node_num) {
	if (node_num >= this->num_nodes) {
		throw invalid_argument("getThreadNum: Cannot submit the node numbered " + to_string(node_num));
	}
	return node_num % num_threads;
}

int MCTS_Shared_Data::numActiveThreads() {
	return num_active_threads;
}

int MCTS_Shared_Data::numActiveMinibatches() {
	return num_active_minibatches;
}

void MCTS_Shared_Data::writeMinibatchToFile(int minibatch_num, string outfile) {
	ofstream f (outfile);
  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			f << to_string(nn_queue->at(state_num)->y) + "\n";
  		}
    	f.close();
  	}
  	else {
  		cout << "Unable to open file";
  	}
}

void MCTS_Shared_Data::unparseNNResults(int minibatch_num, string infile) {
	string line;
	ifstream f (infile);

	int state_num = minibatch_num * minibatch_size;
	while(getline(f, line)) {
		nn_outputs->at(state_num) = new ActionDistribution(stoi(line));
    	state_num++;
    }
}

// make sure thread functions regsiter themselves at beginning of threadFunc
// and master registers itself at beginning
void MCTS_Shared_Data::log(string message) {
	cout_mutex.lock();
	cout << thread_names->at(this_thread::get_id()) << " - " << message << endl;
	cout << message << endl;
	cout_mutex.unlock();
}

void MCTS_Shared_Data::registerThreadName(__thread_id tid, string thread_name) {
	thread_names->insert(pair<__thread_id, string>(tid, thread_name));
}

// testing only
void MCTS_Shared_Data::flipOwnershipFlag(int minibatch_num) {
	shared_data_mutex.lock();
	bool curr = minibatch_ownership->at(minibatch_num);
	bool flip = !curr;
	minibatch_ownership->at(minibatch_num) = flip;
	shared_data_mutex.unlock();
}









MCTS_Node::MCTS_Node(int x) {
	this->x = x;
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
	int new_x = ad->z + node->x;
	node->setX(new_x);
	return new StateVector(new_x);
}

StateVector::StateVector(int y) {
	this->y = y;
}

ActionDistribution::ActionDistribution(int z) {
	this->z = z;
}













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




// what needs to go in the shared state
/* nodes, nn_queue, nn_outputs
array with a slot for each minibatch, true if nn ready, false otherwise
interface with functions isWorkerSafe, isMasterSafe
a condition variable called thread_signaler (workers notify_one the master, master notify_all's the workers) 
an array with a slot for each minibatch, which denotes how many nodes are still active in this minibatch. 
interface with functions numNodesLeftInMinibatch, function nodeComplete (which atomic decrements all necessary counters)
a similar counter array with a slot for each worker thread, which denotes how many nodes are still active in this minibatch
a counter array with slot for each batch, which denotes how many nodes are left to be submitted for this minibatch before master-ready
*/





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