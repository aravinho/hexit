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
	for (int node_num = 0; node_num < num_nodes; node_num++) {
		this->nn_queue->at(node_num) = new StateVector(0);
		this->nn_outputs->at(node_num) = new ActionDistribution(0);
	}

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

	this->current_node_of_thread = new vector<int>(num_threads);
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		this->current_node_of_thread->at(thread_num) = thread_num;
	}

	this->next_active_node = new vector<int>(num_nodes);
	this->prev_active_node = new vector<int>(num_nodes);
	for (int node_num = 0; node_num < num_nodes; node_num++) {
		
		int thread_num = node_num % num_threads;

		// compute the next node, resetting to beginning if necessary
		int next_node_num = node_num + num_threads;
		if (next_node_num >= num_nodes) {
			next_node_num = thread_num;
		}

		// compute the previous node, looping back to end if necessary
		int prev_node_num = node_num - num_threads;
		if (prev_node_num < 0) {
			prev_node_num = num_nodes - (num_threads - thread_num);
		}

		// set the pointers
		this->next_active_node->at(node_num) = next_node_num;
		this->prev_active_node->at(node_num) = prev_node_num;
	}


	this->master_current_minibatch = 0;
	this->next_active_minibatch = new vector<int>(num_minibatches);
	this->prev_active_minibatch = new vector<int>(num_minibatches);
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {

		this->next_active_minibatch->at(minibatch_num) = (minibatch_num + 1) % num_minibatches;
		if (minibatch_num == 0) {
			this->prev_active_minibatch->at(minibatch_num) = num_minibatches - 1;
		}
		else {
			this->prev_active_minibatch->at(minibatch_num) = (minibatch_num - 1) % num_minibatches;
		}
	}




	thread_names = new map<__thread_id, string>();

	// testing only
	this->num_queue_submissions = new vector<int>(num_minibatches, 0);

	// testing having mutexes as pointers

	// promising
	num_submissions_per_thread = new map<int, vector<int>*>();
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		num_submissions_per_thread->insert(pair<int, vector<int>*>(minibatch_num, new vector<int>(num_threads, 0)));
	}


 
}

bool MCTS_Shared_Data::isWorkerSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	bool ret = !(this->minibatch_ownership->at(minibatch_num));
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Shared_Data::isMasterSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	bool ret = this->minibatch_ownership->at(minibatch_num);
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Shared_Data::workerWaitForMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isWorkerSafe(minibatch_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}

	return isMinibatchComplete(minibatch_num);
}

// experimental
bool MCTS_Shared_Data::threadHasAlreadyProcessed(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);
	shared_data_mutex.lock();
	bool already_processed = num_submissions_per_thread->at(minibatch_num)->at(thread_num) >= (minibatch_size / num_threads);
	shared_data_mutex.unlock();
	return already_processed;
}

// experimental
void MCTS_Shared_Data::workerWaitForNode(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);

	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isWorkerSafe(minibatch_num) || threadHasAlreadyProcessed(node_num, thread_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}
}


bool MCTS_Shared_Data::masterWaitForMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isMasterSafe(minibatch_num) && !isMinibatchComplete(minibatch_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}

	return isMinibatchComplete(minibatch_num);
}

bool MCTS_Shared_Data::isMinibatchComplete(int minibatch_num, bool lock_needed) {
	assertValidMinibatchNum(minibatch_num);
	if (lock_needed) {
		shared_data_mutex.lock();
	}
	bool ret = active_nodes_in_minibatch->at(minibatch_num) == 0;
	if (lock_needed) {
		shared_data_mutex.unlock();
	}
	return ret;
}

bool MCTS_Shared_Data::isThreadComplete(int minibatch_num, bool lock_needed) {
	assertValidMinibatchNum(minibatch_num);
	if (lock_needed) {
		shared_data_mutex.lock();
	}
	bool ret = active_nodes_in_thread->at(minibatch_num) == 0;
	if (lock_needed) {
		shared_data_mutex.unlock();
	}
	return ret;
}


MCTS_Node* MCTS_Shared_Data::getNode(int node_num) {
	assertValidNodeNum(node_num);
	return all_nodes->at(node_num);
}

void MCTS_Shared_Data::submitToNNQueue(StateVector* state_vector, int node_num) {
	assertValidNodeNum(node_num);
	if (state_vector == NULL) {
		throw invalid_argument("submitToNNQueue: Cannot submit a NULL state vector for node " + to_string(node_num));
	}

	int minibatch_num = getMinibatchNum(node_num);

	// wait till it's safe to touch this minibatch (it should be already), then submit the state vector
	workerWaitForMinibatch(minibatch_num);
	
	// do we lock this?
	nnQueuePut(state_vector, node_num);
		
	// decrement nodes_left_to_submit for this batch
	// if last node from this batch to submit, mark batch as master safe, and reset counter
	int num_nodes_left = decrementNodesLeft(minibatch_num);

	
}

// testing
void MCTS_Shared_Data::incrementNumQueueSubmissions(int minibatch_num) {
	queue_mutex.lock();
	num_queue_submissions->at(minibatch_num) += 1;
	total_num_queue_submissions += 1;
	queue_mutex.unlock();
}
int MCTS_Shared_Data::numQueueSubmissions(int minibatch_num) {
	queue_mutex.lock();
	int ret = num_queue_submissions->at(minibatch_num);
	queue_mutex.unlock();
	return ret;
}
int MCTS_Shared_Data::totalNumQueueSubmissions() {
	queue_mutex.lock();
	int ret = total_num_queue_submissions;
	queue_mutex.unlock();
	return ret;
}

void MCTS_Shared_Data::submitToNNOutputs(vector<ActionDistribution*>* nn_results, int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	if (nn_results == NULL || nn_results->size() != minibatch_size) {
		throw invalid_argument("submitToNNOutputs: Cannot submit NN Results that are null or the wrong size for minibatch " + to_string(minibatch_num));
	}

	// wait till it's safe to touch this minibatch (it should be already), then submit NN results for every node in this minibatch
	masterWaitForMinibatch(minibatch_num);
	for (int state_num = 0; state_num < minibatch_size; state_num++) {
		int node_num = minibatch_num * minibatch_size + state_num;
		this->nn_outputs->at(node_num) = nn_results->at(state_num);
	}
	// mark this minibatch worker safe
	markWorkerSafe(minibatch_num);

	// notify any (and all) sleeping workers that there is a fresh minibatch to work on
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex); // do we need to unique_lock?
	minibatch_safe_cv.notify_all();

}

void MCTS_Shared_Data::markNodeProcessed(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);

	shared_data_mutex.lock();
	num_submissions_per_thread->at(minibatch_num)->at(thread_num) += 1;
	shared_data_mutex.unlock();
}

void MCTS_Shared_Data::markNodeComplete(int node_num) {
	assertValidNodeNum(node_num);

	int minibatch_num = getMinibatchNum(node_num);
	int thread_num = getThreadNum(node_num);

	// decrement the number of active nodes in this thread and minibatch
	// update the next_active_node and prev_active_node linked list, so in the future we don't bother grabbing this node

	// TO DO: decide whether this lock is causing problems
	//shared_data_mutex.lock();
	active_nodes_in_minibatch->at(minibatch_num) -= 1;

	bool minibatch_complete = isMinibatchComplete(minibatch_num, false);

	if (minibatch_complete) {
		num_active_minibatches--;
		// if the completion of this node completes the minibatch, update the next_active_minibatch and prev_active_minibatch linked list
		// so the master won't waste time looking at completed minibatches
		int prev_minibatch = prev_active_minibatch->at(minibatch_num);
		int next_minibatch = next_active_minibatch->at(minibatch_num);
		next_active_minibatch->at(prev_minibatch) = next_minibatch;
		prev_active_minibatch->at(next_minibatch) = prev_minibatch;

	}

	active_nodes_in_thread->at(thread_num) -= 1;

	bool thread_complete = isThreadComplete(thread_num, false);
	if (thread_complete) {
		num_active_threads--;
	}


	int prev_node = prev_active_node->at(node_num);
	int next_node = next_active_node->at(node_num);

	next_active_node->at(prev_node) = next_node;
	prev_active_node->at(next_node) = prev_node;

	//decrement the number of nodes left to submit for this batch
	decrementNodesLeft(minibatch_num, false);
	
	// TO DO
	//shared_data_mutex.unlock();
}

// removing lock and unlock, because is only called from locked context
void MCTS_Shared_Data::markMasterSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	minibatch_ownership->at(minibatch_num) = true;
}

void MCTS_Shared_Data::markWorkerSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	minibatch_ownership->at(minibatch_num) = false;
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		num_submissions_per_thread->at(minibatch_num)->at(thread_num) = 0;
	}
	shared_data_mutex.unlock();
}

void MCTS_Shared_Data::logMinibatch(int minibatch_num) {
	cout_mutex.lock();
	cout << endl << thread_names->at(this_thread::get_id()) << " - ";
	cout <<  "Batch " << minibatch_num << endl;

	cout << endl << "StateVector Y-Values: " << endl;
	for (int node_num = minibatch_num * minibatch_size; node_num < (minibatch_num + 1) * minibatch_size; node_num++) {
		cout << nnQueueGet(node_num)->y << "\t";
	}

	cout << endl << endl;;
	cout_mutex.unlock();
}

void MCTS_Shared_Data::logQueue() {
	cout_mutex.lock();
	cout << endl << "Queue:" << endl;
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		cout << "Batch " << minibatch_num << ":\t";
		for (int node_num = minibatch_num * minibatch_size; node_num < (minibatch_num + 1) * minibatch_size; node_num++) {
			cout << nnQueueGet(node_num)->y << "\t";
		}
		cout << endl << endl;
	}
	cout_mutex.unlock();
}


void MCTS_Shared_Data::logNumQueueSubmissions() {
	/*cout_mutex.lock();
	cout << "Total number of submissions: " << totalNumQueueSubmissions() << endl;
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		cout << numQueueSubmissions(minibatch_num) << "\t";
	}
	cout << endl;
	cout_mutex.unlock();*/


	cout_mutex.lock();
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		cout << "Batch " << minibatch_num << ":\t";
		for (int thread_num = 0; thread_num < num_threads; thread_num++) {
			cout << num_submissions_per_thread->at(minibatch_num)->at(thread_num) << "\t";
		}
		cout << endl;
	}
	cout << endl;
	cout_mutex.unlock();
}

int MCTS_Shared_Data::decrementNodesLeft(int minibatch_num, bool lock_needed) {
	assertValidMinibatchNum(minibatch_num);

	// enter atomic zone
	if (lock_needed) {
		shared_data_mutex.lock();
	}

	// grab the number of nodes in this minibatch left to submit to the NN queue, and subtract 1 (Read only)
	int prev_num_nodes_left = nodes_left_to_submit->at(minibatch_num);
	int num_nodes_left = prev_num_nodes_left - 1;
	bool minibatch_nn_ready = (num_nodes_left == 0);

	// if this is the last node from this batch to be submitted, reset that number to the number of active nodes in the minibatch
	if (minibatch_nn_ready) {
		num_nodes_left = active_nodes_in_minibatch->at(minibatch_num);
	}

	// make sure there's no error
	if (num_nodes_left < 0) {
		throw logic_error("There are now fewer than 0 nodes left to submit for the minibatch numbered " + to_string(minibatch_num));
	}

	// if the node is master-ready, mark it as such
	if (minibatch_nn_ready) {
		markMasterSafe(minibatch_num);
	}

	// now perform the actual update to the nodes_left_to_submit counter variable (Write operation)
	nodes_left_to_submit->at(minibatch_num) = num_nodes_left;	
	
	if (lock_needed) {
		shared_data_mutex.unlock();
	}

	// if node is master-ready notify the master
	if (minibatch_nn_ready) {
		unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex); // do we need to unique_lock?
		minibatch_safe_cv.notify_one();
	}

	

	return num_nodes_left;

}

StateVector* MCTS_Shared_Data::nnQueueGet(int node_num) {
	queue_mutex.lock();
	StateVector* ret = nn_queue->at(node_num);
	queue_mutex.unlock();
	return ret;

}

void MCTS_Shared_Data::nnQueuePut(StateVector* state_vector, int node_num) {
	queue_mutex.lock();
	nn_queue->at(node_num) = state_vector;
	num_queue_submissions->at(getMinibatchNum(node_num)) += 1;
	total_num_queue_submissions += 1;
	queue_mutex.unlock();
}


int MCTS_Shared_Data::getMinibatchNum(int node_num) {
	assertValidNodeNum(node_num);
	return node_num / minibatch_size;
}

int MCTS_Shared_Data::getThreadNum(int node_num) {
	assertValidNodeNum(node_num);
	return node_num % num_threads;
}

int MCTS_Shared_Data::numActiveThreads() {
	return num_active_threads;
}

int MCTS_Shared_Data::numActiveMinibatches() {
	return num_active_minibatches;
}

int MCTS_Shared_Data::numActiveNodesInThread(int thread_num) {
	assertValidThreadNum(thread_num);
	return active_nodes_in_thread->at(thread_num);
}

int MCTS_Shared_Data::numActiveNodesInMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	return active_nodes_in_minibatch->at(minibatch_num);
}

void MCTS_Shared_Data::assertValidThreadNum(int thread_num) {
	if (thread_num < 0 || thread_num >= num_threads) {
		throw invalid_argument("No thread numbered " + to_string(thread_num));
	}
}

void MCTS_Shared_Data::assertValidMinibatchNum(int minibatch_num) {
	if (minibatch_num < 0 || minibatch_num >= num_minibatches) {
		throw invalid_argument("No minibatch numbered " + to_string(minibatch_num));
	}
}

void MCTS_Shared_Data::assertValidNodeNum(int node_num) {
	if (node_num < 0 || node_num >= num_nodes) {
		throw invalid_argument("No node numbered " + to_string(node_num));
	}
}

int MCTS_Shared_Data::nextActiveNodeNum(int thread_num) {
	assertValidThreadNum(thread_num);
	if (numActiveNodesInThread(thread_num) <= 0) {
		return -1;
	}

	int curr_node_num = current_node_of_thread->at(thread_num);
	int next_node_num = next_active_node->at(curr_node_num);
	current_node_of_thread->at(thread_num) = next_node_num;
	return curr_node_num;

}

int MCTS_Shared_Data::nextActiveMinibatchNum() {
	if (num_active_minibatches == 0) {
		return -1;
	}

	int ret = master_current_minibatch;
	int next_minibatch_num = next_active_minibatch->at(master_current_minibatch);
	master_current_minibatch = next_minibatch_num;
	return ret;

}


void MCTS_Shared_Data::writeMinibatchToFile(int minibatch_num, string outfile) {
	ofstream f (outfile);
  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			f << to_string(nnQueueGet(state_num)->y) << "\n";
  		}
    	f.close();
  	}
  	else {
  		throw invalid_argument("Unable to open file " + outfile);
  	}
}

void MCTS_Shared_Data::writeMinibatchNodesToFile(int minibatch_num, string outfile) {
	ofstream f (outfile);
  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			f << to_string(all_nodes->at(state_num)->x) << "\n";
  		}
    	f.close();
  	}
  	else {
  		throw invalid_argument("Unable to open file " + outfile);
  	}
}

void MCTS_Shared_Data::unparseNNResults(int minibatch_num, string infile) {
	string line;
	ifstream f (infile);

	vector<ActionDistribution*>* minibatch_nn_outputs = new vector<ActionDistribution*>(minibatch_size);
	int state_num = 0;
	while(getline(f, line)) {
		minibatch_nn_outputs->at(state_num) = new ActionDistribution(stoi(line));
		state_num++;
    }

    submitToNNOutputs(minibatch_nn_outputs, minibatch_num);
}

ActionDistribution* MCTS_Shared_Data::getNNOutput(int node_num) {
	assertValidNodeNum(node_num);
	workerWaitForMinibatch(getMinibatchNum(node_num));
	return nn_outputs->at(node_num);
}

StateVector* MCTS_Shared_Data::getStateVector(int node_num) {
	assertValidNodeNum(node_num);
	masterWaitForMinibatch(getMinibatchNum(node_num));
	return nnQueueGet(node_num);
}

// make sure thread functions regsiter themselves at beginning of threadFunc
// and master registers itself at beginning
void MCTS_Shared_Data::log(string message) {
	cout_mutex.lock();
	/*string thread_name = thread_names->at(this_thread::get_id());
	
	int num_spaces;
	string offset = "";
	if (thread_name == "Worker 0") num_spaces = 0;
	if (thread_name == "Worker 1") num_spaces = 80;
	if (thread_name == "Worker 2") num_spaces = 160;
	if (thread_name == "Worker 3") num_spaces = 240;
	for (int s = 0; s < num_spaces; s++) {
		offset += " ";
	}*/
	string offset = "";

	cout << offset << thread_names->at(this_thread::get_id()) << " - " << message << endl;
	cout_mutex.unlock();
}

void MCTS_Shared_Data::registerThreadName(__thread_id tid, string thread_name) {
	cout_mutex.lock();
	thread_names->insert(pair<__thread_id, string>(tid, thread_name));
	cout_mutex.unlock();
}











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
	if (ad->z != 0 && ad->z != node->x) {
		cout << "YELL, node " << node->init_x << " is messed. ad->Z = " << ad->z << ",  node->x = " << node->x << endl;
	}
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

		// read in the resulting NN action distributions and write them into the NN output queue
		data->unparseNNResults(minibatch_num, action_distribution_file);

		//data->log("Master finished with minibatch " + to_string(minibatch_num) + " in round " + to_string(round));


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





