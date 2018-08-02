#include "mcts_thread_manager.h"

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

bool logging = true;

MCTS_Thread_Manager::MCTS_Thread_Manager(vector<MCTS_Node*>* all_nodes, int num_nodes, int minibatch_size, int num_threads, int num_actions) {
	if (num_nodes % minibatch_size != 0) {
		throw invalid_argument("The minibatch size must be divisible by the total number of nodes");
	}
	if (num_nodes % num_threads != 0) {
		throw invalid_argument("The number of worker threads must be divisible by the total number of nodes");
	}

	this->num_actions = num_actions;
	this->num_nodes = num_nodes;
	this->minibatch_size = minibatch_size;
	this->num_threads = num_threads;
	this->num_minibatches = num_nodes / minibatch_size;
	this->num_nodes_per_thread = num_nodes / num_threads;

	this->all_nodes = all_nodes;
	this->nn_queue = new vector<StateVector*>(num_nodes);
	this->nn_results = new vector<ActionDistribution*>(num_nodes);
	// initialize all StateVectors and ActionDistributions to dummies
	for (int node_num = 0; node_num < num_nodes; node_num++) {
		this->nn_queue->at(node_num) = new StateVector();
		this->nn_results->at(node_num) = new ActionDistribution();
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

	this->num_active_minibatches = num_minibatches;
	this->num_active_threads = num_threads;

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

	// for logging
	thread_names = new map<__thread_id, string>();

	num_submissions_per_thread = new map<int, vector<int>*>();
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		num_submissions_per_thread->insert(pair<int, vector<int>*>(minibatch_num, new vector<int>(num_threads, 0)));
	}

	left_to_submit_per_thread = new map<int, vector<int>*>();
	active_nodes_in_minibatch_per_thread = new map<int, vector<int>*>();
	for (int minibatch_num = 0; minibatch_num < num_minibatches; minibatch_num++) {
		left_to_submit_per_thread->insert(pair<int, vector<int>*>(minibatch_num, new vector<int>(num_threads, minibatch_size / num_threads)));
		active_nodes_in_minibatch_per_thread->insert(pair<int, vector<int>*>(minibatch_num, new vector<int>(num_threads, minibatch_size / num_threads)));
	}

	// for bookkeeping
	num_script_invocations = 0;
 
}








bool MCTS_Thread_Manager::workerWaitForMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isWorkerSafe(minibatch_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}

	return isMinibatchComplete(minibatch_num);
}

void MCTS_Thread_Manager::workerWaitForNode(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isWorkerSafe(minibatch_num) || threadHasAlreadyProcessed(node_num, thread_num)) {
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}
	
}

bool MCTS_Thread_Manager::masterWaitForMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex);
	while (!isMasterSafe(minibatch_num) && !isMinibatchComplete(minibatch_num)) {
		
		minibatch_safe_cv.wait(minibatch_safe_lock);
	}
	return isMinibatchComplete(minibatch_num);
}






int MCTS_Thread_Manager::nextActiveNodeNum(int thread_num) {
	assertValidThreadNum(thread_num);
	if (numActiveNodesInThread(thread_num) <= 0) {
		return -1;
	}

	int curr_node_num = current_node_of_thread->at(thread_num);
	int next_node_num = next_active_node->at(curr_node_num);
	current_node_of_thread->at(thread_num) = next_node_num;
	return curr_node_num;

}

int MCTS_Thread_Manager::nextActiveMinibatchNum() {
	if (num_active_minibatches == 0) {
		return -1;
	}

	int ret = master_current_minibatch;
	int next_minibatch_num = next_active_minibatch->at(master_current_minibatch);
	master_current_minibatch = next_minibatch_num;
	return ret;

}






vector<MCTS_Node*>* MCTS_Thread_Manager::getAllNodes() {
	return all_nodes;
}

MCTS_Node* MCTS_Thread_Manager::getNode(int node_num) {
	assertValidNodeNum(node_num);
	return all_nodes->at(node_num);
}

void MCTS_Thread_Manager::setNode(int node_num, MCTS_Node* node) {
	assertValidNodeNum(node_num);
	all_nodes->at(node_num) = node;
}


StateVector* MCTS_Thread_Manager::getStateVector(int node_num) {
	assertValidNodeNum(node_num);
	masterWaitForMinibatch(getMinibatchNum(node_num));
	return nnQueueGet(node_num);
}

ActionDistribution* MCTS_Thread_Manager::getNNResult(int node_num) {
	assertValidNodeNum(node_num);
	workerWaitForMinibatch(getMinibatchNum(node_num));
	return adQueueGet(node_num);
}

void MCTS_Thread_Manager::submitToNNQueue(StateVector* state_vector, int node_num) {
	assertValidNodeNum(node_num);
	if (state_vector == NULL) {
		throw invalid_argument("submitToNNQueue: Cannot submit a NULL state vector for node " + to_string(node_num));
	}
	int minibatch_num = getMinibatchNum(node_num);

	// wait till it's safe to touch this minibatch (it should be already), then submit the state vector
	workerWaitForMinibatch(minibatch_num);
	nnQueuePut(state_vector, node_num);
		
	// decrement nodes_left_to_submit for this batch
	// if last node from this batch to submit, mark batch as master safe, and reset counter
	int num_nodes_left = decrementNodesLeft(minibatch_num, node_num);
	
}


void MCTS_Thread_Manager::submitToNNResults(vector<ActionDistribution*>* nn_results, int minibatch_num, int round) {
	assertValidMinibatchNum(minibatch_num);
	if (nn_results == NULL || nn_results->size() != minibatch_size) {
		throw invalid_argument("submitToNNResults: Cannot submit NN Results that are null or the wrong size for minibatch " + to_string(minibatch_num));
	}
	// wait till it's safe to touch this minibatch (it should be already), then submit NN results for every node in this minibatch
	masterWaitForMinibatch(minibatch_num);


	for (int state_num = 0; state_num < minibatch_size; state_num++) {
		int node_num = minibatch_num * minibatch_size + state_num;
		adQueuePut(nn_results->at(state_num), node_num);
	}

	// mark this minibatch worker safe
	markWorkerSafe(minibatch_num);

	// notify any (and all) sleeping workers that there is a fresh minibatch to work on
	unique_lock<mutex> minibatch_safe_lock(minibatch_safe_mutex); // do we need to unique_lock?
	minibatch_safe_cv.notify_all();

}






void MCTS_Thread_Manager::markNodeProcessed(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);

	shared_data_mutex.lock();
	num_submissions_per_thread->at(minibatch_num)->at(thread_num) += 1;
	left_to_submit_per_thread->at(minibatch_num)->at(thread_num) -= 1;
	shared_data_mutex.unlock();
}

void MCTS_Thread_Manager::markNodeComplete(int node_num) {
	assertValidNodeNum(node_num);

	int minibatch_num = getMinibatchNum(node_num);
	int thread_num = getThreadNum(node_num);

	// decrement the number of active nodes in this thread and minibatch
	// update the next_active_node and prev_active_node linked list, so in the future we don't bother grabbing this node

	int curr_num_active_nodes = active_nodes_in_minibatch->at(minibatch_num);
	int new_num_active_nodes = curr_num_active_nodes - 1;
	active_nodes_in_minibatch->at(minibatch_num) = new_num_active_nodes;

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
	active_nodes_in_minibatch_per_thread->at(minibatch_num)->at(thread_num) -= 1;

	bool thread_complete = isThreadComplete(thread_num, false);
	if (thread_complete) {
		num_active_threads--;
	}


	int prev_node = prev_active_node->at(node_num);
	int next_node = next_active_node->at(node_num);

	next_active_node->at(prev_node) = next_node;
	prev_active_node->at(next_node) = prev_node;

	//decrement the number of nodes left to submit for this batch
	decrementNodesLeft(minibatch_num, node_num);
	
}




void MCTS_Thread_Manager::serializeNNQueue(int minibatch_num, string outfile) {
	ofstream f (outfile);

  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			
  			//StateVector* sv = nnQueueGet(state_num);
  			//string csv_string = sv->asCSVString(tmp_log); // turn on log for now
  			//f << csv_string << "\n";
  			f << nnQueueGet(state_num)->asCSVString() << "\n";
  			
  		}
    	f.close();
  	}
  	else {
  		throw invalid_argument("In serializeNNQueue, Unable to open file " + outfile);
  	}
}

/*
void MCTS_Thread_Manager::serializeNNQueue(int minibatch_num, string outfile) {
	ofstream f (outfile);
  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			f << to_string(nnQueueGet(state_num)->y) << "\n";
  		}
    	f.close();
  	}
  	else {
  		throw invalid_argument("In serializeNNQueue, Unable to open file " + outfile);
  	}
}*/

/*void MCTS_Thread_Manager::writeNodesToFile(int minibatch_num, string outfile) {
	ofstream f (outfile);
  	if (f.is_open()) {
  		for (int state_num = minibatch_num * minibatch_size; state_num < (minibatch_num + 1) * minibatch_size; state_num++) {
  			f << to_string(all_nodes->at(state_num)->x) << "\n";
  		}
    	f.close();
  	}
  	else {
  		throw invalid_argument("In writeNodesToFile, Unable to open file " + outfile);
  	}
}*/

void MCTS_Thread_Manager::deserializeNNResults(int minibatch_num, string infile, int round) {
	string line;
	ifstream f (infile);

	if (f.is_open()) {
		vector<ActionDistribution*>* minibatch_nn_results = new vector<ActionDistribution*>(minibatch_size);
		int state_num = 0;
		while(getline(f, line)) {
			//minibatch_nn_results->at(state_num) = new ActionDistribution(stoi(line));
			minibatch_nn_results->at(state_num) = new ActionDistribution(this->num_actions, line);

			state_num++;
	    }
	    f.close();

	    submitToNNResults(minibatch_nn_results, minibatch_num, round);
	}
	else {
		throw invalid_argument("In deserializeNNResults, Unable to open file " + infile);
	}

}

/*void MCTS_Thread_Manager::deserializeNNResults(int minibatch_num, string infile, int round) {
	string line;
	ifstream f (infile);

	if (f.is_open()) {
		vector<ActionDistribution*>* minibatch_nn_results = new vector<ActionDistribution*>(minibatch_size);
		int state_num = 0;
		while(getline(f, line)) {
			minibatch_nn_results->at(state_num) = new ActionDistribution(stoi(line));
			state_num++;
	    }
	    f.close();

	    submitToNNResults(minibatch_nn_results, minibatch_num, round);
	}
	else {
		throw invalid_argument("In deserializeNNResults, Unable to open file " + infile);
	}

}*/




// make sure thread functions regsiter themselves at beginning of threadFunc
// and master registers itself at beginning
void MCTS_Thread_Manager::log(string message, bool force, bool suppress) {
	if (suppress && !force) return;
	if (!logging && !force) return;
	cout_mutex.lock();
	string offset = "";
	cout << offset << thread_names->at(this_thread::get_id()) << " - " << message << endl;
	cout_mutex.unlock();
}

void MCTS_Thread_Manager::registerThreadName(__thread_id tid, string thread_name) {
	cout_mutex.lock();
	thread_names->insert(pair<__thread_id, string>(tid, thread_name));
	cout_mutex.unlock();
}

void MCTS_Thread_Manager::invokePythonScript(string script_file, string infile, string outfile) {
	shared_data_mutex.lock();
	num_script_invocations++;
	shared_data_mutex.unlock();

	char *command = (char *) "python";
	char script_name[100]; strcpy(script_name, script_file.c_str());
	char infile_arg[100]; strcpy(infile_arg, infile.c_str());
	char outfile_arg[100]; strcpy(outfile_arg, outfile.c_str());
    char *pythonArgs[]={command, script_name, infile_arg, outfile_arg, NULL};

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

void MCTS_Thread_Manager::invokeNNScript(string script_file, string state_vector_file, string model_dirname, string action_distribution_file) {
	shared_data_mutex.lock();
	num_script_invocations++;
	shared_data_mutex.unlock();

	char *command = (char *) "python";
	char script_name[100]; strcpy(script_name, script_file.c_str());
	char infile_arg[100]; strcpy(infile_arg, state_vector_file.c_str());
	char modeldir_arg[200]; strcpy(modeldir_arg, model_dirname.c_str());
	char outfile_arg[100]; strcpy(outfile_arg, action_distribution_file.c_str());
    char *pythonArgs[]={command, script_name, infile_arg, modeldir_arg, outfile_arg, NULL};

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






/*** Private methods below ****/



bool MCTS_Thread_Manager::isWorkerSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	bool ret = !(this->minibatch_ownership->at(minibatch_num));
	shared_data_mutex.unlock();
	return ret;
}

bool MCTS_Thread_Manager::isMasterSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	bool ret = this->minibatch_ownership->at(minibatch_num);
	shared_data_mutex.unlock();
	return ret;
}


bool MCTS_Thread_Manager::isMinibatchComplete(int minibatch_num, bool lock_needed) {
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

bool MCTS_Thread_Manager::isThreadComplete(int minibatch_num, bool lock_needed) {
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






void MCTS_Thread_Manager::markMasterSafe(int minibatch_num, bool from_locked) {
	assertValidMinibatchNum(minibatch_num);
	minibatch_ownership->at(minibatch_num) = true;
}

void MCTS_Thread_Manager::markWorkerSafe(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	shared_data_mutex.lock();
	minibatch_ownership->at(minibatch_num) = false;
	for (int thread_num = 0; thread_num < num_threads; thread_num++) {
		num_submissions_per_thread->at(minibatch_num)->at(thread_num) = 0;
		left_to_submit_per_thread->at(minibatch_num)->at(thread_num) = 
			active_nodes_in_minibatch_per_thread->at(minibatch_num)->at(thread_num);
	}
	shared_data_mutex.unlock();
}


int MCTS_Thread_Manager::decrementNodesLeft(int minibatch_num, int node_num, bool lock_needed) {
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
		//log("num_nodes_left updated from 0 to " + to_string(num_nodes_left) + ln);
	}

	// make sure there's no error
	if (num_nodes_left < 0) {
		throw logic_error("There are now fewer than 0 nodes left to submit for the minibatch numbered " + to_string(minibatch_num));
	}

	// if the node is master-ready, mark it as such
	if (minibatch_nn_ready) {
		markMasterSafe(minibatch_num, lock_needed);
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

bool MCTS_Thread_Manager::threadHasAlreadyProcessed(int node_num, int thread_num) {
	assertValidNodeNum(node_num);
	assertValidThreadNum(thread_num);

	int minibatch_num = getMinibatchNum(node_num);
	shared_data_mutex.lock();

	int left_to_process = left_to_submit_per_thread->at(minibatch_num)->at(thread_num);
	int active_nodes_in_thread = active_nodes_in_minibatch_per_thread->at(minibatch_num)->at(thread_num);
	bool safe_to_process = true;

	if (active_nodes_in_thread > 0) {
		if (left_to_process == 0) {
			safe_to_process = false;
		}
	}

	bool already_processed = !safe_to_process;

	shared_data_mutex.unlock();
	return already_processed;
}










int MCTS_Thread_Manager::getMinibatchNum(int node_num) {
	assertValidNodeNum(node_num);
	return node_num / minibatch_size;
}

int MCTS_Thread_Manager::getThreadNum(int node_num) {
	assertValidNodeNum(node_num);
	return node_num % num_threads;
}

int MCTS_Thread_Manager::numActiveThreads() {
	return num_active_threads;
}

int MCTS_Thread_Manager::numActiveMinibatches() {
	return num_active_minibatches;
}

int MCTS_Thread_Manager::numActiveNodesInThread(int thread_num) {
	assertValidThreadNum(thread_num);
	return active_nodes_in_thread->at(thread_num);
}

int MCTS_Thread_Manager::numActiveNodesInMinibatch(int minibatch_num) {
	assertValidMinibatchNum(minibatch_num);
	return active_nodes_in_minibatch->at(minibatch_num);
}

void MCTS_Thread_Manager::assertValidThreadNum(int thread_num) {
	if (thread_num < 0 || thread_num >= num_threads) {
		throw invalid_argument("No thread numbered " + to_string(thread_num));
	}
}

void MCTS_Thread_Manager::assertValidMinibatchNum(int minibatch_num) {
	if (minibatch_num < 0 || minibatch_num >= num_minibatches) {
		throw invalid_argument("No minibatch numbered " + to_string(minibatch_num));
	}
}

void MCTS_Thread_Manager::assertValidNodeNum(int node_num) {
	if (node_num < 0 || node_num >= num_nodes) {
		throw invalid_argument("No node numbered " + to_string(node_num));
	}
}








ActionDistribution* MCTS_Thread_Manager::adQueueGet(int node_num) {
	queue_mutex.lock();
	ActionDistribution* ret = nn_results->at(node_num);
	queue_mutex.unlock();
	return ret;

}

void MCTS_Thread_Manager::adQueuePut(ActionDistribution* ad, int node_num) {
	queue_mutex.lock();
	nn_results->at(node_num) = ad;
	// should delete the existing one
	queue_mutex.unlock();
}

StateVector* MCTS_Thread_Manager::nnQueueGet(int node_num) {
	queue_mutex.lock();
	StateVector* ret = nn_queue->at(node_num);
	queue_mutex.unlock();
	return ret;

}

void MCTS_Thread_Manager::nnQueuePut(StateVector* state_vector, int node_num) {
	queue_mutex.lock();
	nn_queue->at(node_num) = state_vector;
	// maybe delete existing state vector
	queue_mutex.unlock();
}





















