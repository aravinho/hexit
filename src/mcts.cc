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



using namespace std;

int NUM_NODES = 64;
int BATCH_SIZE = 16; // NN batch size


mutex cout_mutex, nodes_pushed_mutex;

std::mutex m_mutex;
std::condition_variable m_condVar;


 int nodes_pushed = 0;
// initialize nodes and queue
vector<int> nodes(NUM_NODES);
vector<int> queue(NUM_NODES);
vector<bool> worker_safe(NUM_NODES);
vector<int> batch_counters(NUM_NODES / BATCH_SIZE);
vector<int> batch_ready(NUM_NODES / BATCH_SIZE);

map<__thread_id, string> thread_nums;

int num_batches_ready = 0;
mutex batch_ready_lock;


void threadSafePrint(string str) {
	cout_mutex.lock();
	cout << thread_nums[this_thread::get_id()] << " - " << str << endl;
	cout_mutex.unlock();
}

int updateCounter(int node_num, mutex& mtx) {
	mtx.lock();
	batch_counters[node_num / BATCH_SIZE]++;
	int counter = batch_counters[node_num / BATCH_SIZE];
	mtx.unlock();
	return counter;
}

void workerThreadFunc(int thread_num) {
	// process my thread
	for (int i = thread_num; i < NUM_NODES; i += 4) {
		// make sure this node is marked as worker_safe
		if (!worker_safe[i]) {
			continue;
		}

		// do some computation with the node (tree exploration)
		nodes[i] = i;

		// eventually reach a stage where we need to submit to queue
		queue[i] = nodes[i];

		// update the counter of nodes submitted
		int ctr = updateCounter(i, nodes_pushed_mutex);
		//threadSafePrint("Updated the node #" + to_string(i) + ". Now the counter is " + to_string(ctr));
		
		if (ctr == BATCH_SIZE) {
			int batch_completed = i / BATCH_SIZE;
			
			//batch_ready[i / BATCH_SIZE] = true;
			
			batch_ready_lock.lock();
			threadSafePrint("Batch " + to_string(batch_completed) + " complete. Current num_batches_ready: " + to_string(num_batches_ready));
			num_batches_ready += 1;
	    	threadSafePrint("INCREMENTING num_batches_ready to " + to_string(num_batches_ready));
			batch_ready_lock.unlock();
			// wake up the master
			//lock_guard<mutex> guard(m_mutex);
   			// Notify the condition variable
   			m_condVar.notify_one();

		}

		
	}
	threadSafePrint("WORKER THREAD FINISHING: " + to_string(thread_num));
	/*cout_mutex.lock();
	cout << "I am a worker thread! " << this_thread::get_id() << endl;
	cout_mutex.unlock();*/
}

bool batchReady() {
	return num_batches_ready > 0; //return batch_ready[batch_num];
}

void runPythonScript() {

	// start here
	for (int t = 0; t < 1; t++) {
		cout << "t: " << t << endl;
		// read the next batch of state vectors
		for (int i = 0; i < NUM_NODES; i++) {
			nodes[i] = i;
			queue[i] = 0;
			worker_safe[i] = true;
			batch_ready[i / BATCH_SIZE] = false;
		}

		// initialize the nodes_pushed counter
		nodes_pushed = 0;

		// register this thread
		thread_nums[this_thread::get_id()] = "MASTER";

		// Spawn worker threads
		thread worker_threads[4];
		for (int i = 0; i < 4; i++) {
			worker_threads[i] = thread(workerThreadFunc, i);
			thread_nums[worker_threads[i].get_id()] = "Worker " + to_string(i);
		}

		// wait to be signaled by a worker threadm
		unique_lock<mutex> mlock(m_mutex);
		for (int b = 0; b < NUM_NODES / BATCH_SIZE; b++) {
			
			threadSafePrint("MASTER WAITING");
	    	// Start waiting for the Condition Variable to get signaled
	    	// Wait() will internally release the lock and make the thread to block
	   		// As soon as condition variable get signaled, resume the thread and
	    	// again acquire the lock. Then check if condition is met or not
	    	// If condition is met then continue else again go in wait.
	    	m_condVar.wait(mlock, batchReady);

	    	threadSafePrint("MASTER AWAKENED, Ready to process batch " + to_string(b));
	    		    
			

			char *command = (char *) "python";
			char *scriptName= (char *) "src/script.py";  // it can also be resolved using your PATH environment variable
			char *batchNum = new char[2];
			strcpy(batchNum, to_string(b).c_str());
		    char *pythonArgs[]={command, scriptName, batchNum, NULL};

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

		    threadSafePrint("FINSIHED CALLING SCRIPT FOR THE " + to_string(b) + "th time");


	    	// decrement number of ready batches
	    	batch_ready_lock.lock();
	    	threadSafePrint("BEFORE DECREMENT, num_batches_ready = " + to_string(num_batches_ready));
	    	num_batches_ready -= 1;
	    	threadSafePrint("Decrementing num_batches_ready to " + to_string(num_batches_ready));
	    	batch_ready_lock.unlock();

	 	}


	    // wait for workers to join
		for (int i = 0; i < 4; i++) {
			worker_threads[i].join();
		}

		threadSafePrint("All worker threads joined");
	}
}