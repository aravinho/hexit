#ifndef MCTS_SHARED_DATA_H

#include "mcts_node.h"
#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>


using namespace std;


/* This class contains all the data which is shared between master and worker threads.
 * This includes the arrays of nodes, the queue of state vectors, and the queue of NN results.
 * Also includes flags and counter variables that mark the progress of each thread/mini-batch,
 * and prevent multiple threads from touching the same area.
 */
class MCTS_Shared_Data {

public:

	/* Initializes all the shared data to the appropriate initial values. */
	MCTS_Shared_Data(vector<MCTS_Node*>* all_nodes, int num_nodes=64, int minibatch_size=16, int num_threads=4);

	/* Called by Worker thread. Returns true if the given minibatch is safe for workers to process. */
	bool isWorkerSafe(int minibatch_num);

	/* Called by Master thread. Returns true if the given minibatch is safe for the master to process. */
	bool isMasterSafe(int minibatch_num);

	/* Called by either a Worker or a Master thread.  Returns true if the given minibatch is permanently complete, else false. 
	 * lock_needed is set to true if the calling thread already holds the shared_data_mutex lock. */
	bool isMinibatchComplete(int minibatch_num, bool lock_needed=true);

	/* Called by either a Worker or a Master thread.  Returns true if all the nodes that the given thread is responsible for are complete. */
	bool isThreadComplete(int minibatch_num, bool lock_needed=true);

	/* Called by Worker Thread. Causes the current worker thread to wait until it is safe to process nodes in this minibatch.
	 * Returns true if the minibatch is permanently complete, and false if there are still nodes that need work. */ 
	bool workerWaitForMinibatch(int minibatch_num);

	/* Called by Master Thread. Causes the master thread to wait until either:
	* - it is safe for the master to process nodes in this minibatch, OR
	* - the minibatch is permanently completed.
	* Returns true if the minibatch is permenantly complete, and false if there are still nodes that need work. */
	bool masterWaitForMinibatch(int minibatch_num);

	/* Called by Worker Thread.
	 * Submits the given state_vector, which corresponds to the node indexed by node_num.
	 * Places this state_vector into the nn_queue, and increments the ready-counters for this minibatch.
	 * If the submission of this state vector causes its minibatch to become NN-ready, sets the appropriate flags,
	 * notifies the master thread, and then resets the appropriate counter variables to prepare for the next round of processing. */
	void submitToNNQueue(StateVector* state_vector, int node_num);

	/* Called by worker thread.  Returns a pointer to the node indexed by node_num. */
	MCTS_Node* getNode(int node_num);

	/* Called by worker thread. Returns a pointer to the Action Distribution (from the nn_outputs vector) indexed by node_num. */
	ActionDistribution* getNNOutput(int node_num);

	/* Called by master thread. Returns a pointer to the StateVector (from the nn_queue) indexed by node_num. */
	StateVector* getStateVector(int node_num);

	/* Called by Master thread, once the NN results for the given minibatch are safely written to the nn_outputs vector.
	 * This resets appropriate flags, releases the master's hold on this minibatch, and signals to workers that there is a fresh minibatch for them to work on. */
	void submitToNNOutputs(vector<ActionDistribution*>* nn_results, int minibatch_num, int round);

	/* Called by Worker thread, when a node has permanently finished its computation.
	 * Decrements the active-node counters for this node's minibatch and thread. */
	void markNodeComplete(int node_num);

	/* Called by master or worker. Returns the number of active threads. */
	int numActiveThreads();

	/* Called by worker threads. Returns the number of active nodes left for this thread. */
	int numActiveNodesInThread(int thread_num);

	/* Called by worker or master thread.  Returns the number of active nodes left in this minibatch. */
	int numActiveNodesInMinibatch(int minibatch_num);

	/* Called by master or worker. Returns the number of active minibatches. */
	int numActiveMinibatches();

	/* Called by the master thread.  Grabs the given minibatch from the nn_queue, converts each StateVector into a serializable format, and writes it to file. */
	void writeMinibatchToFile(int minibatch_num, string outfile);

	/* Called by the master thread at very end.  Grabs the given minibatch from the nodes vector, and writes each node to file. */
	void writeMinibatchNodesToFile(int minibatch_num, string outfile);

	/* Called by the master thread.  Reads the given file, unpacks it into a vector of ActionDistributions, and writes these to the nn_outputs queue. */
	void unparseNNResults(int minibatch_num, string infile, int round=0);

	/* Returns the index of the minibatch corresponding to the given node. */
	int getMinibatchNum(int node_num);

	/* Returns the index of the thread responsible for the given node. */
	int getThreadNum(int node_num);

	/* Called by worker thread.
	 * Returns the index of the next active node for the given thread.  This function acts like an iterator.
	 * Cycles back to the thread's first node once it reaches the end.
	 * Returns -1 if there are no more nodes to process.
	 */
	int nextActiveNodeNum(int thread_num);

	/* Called by master thread.
	 * Returns the index of the next active minibatch.  This function acts like an iterator.
	 * Cycles back to the first active minibatch once it reaches the end.
	 * Returns -1 if all minibatches are complete.
	 */
	int nextActiveMinibatchNum();

	/* Called by master or worker thread.  Logs the given message after acquiring the cout_lock. */
	void log(string message, bool force=false);

	/* Called by master or worker thread to register a name for itself. For logging purposes. */
	void registerThreadName(__thread_id tid, string thread_name);

	// for testing, prints a minibatch
	void logMinibatch(int minibatch_num);

	// if works, comment nicely
	StateVector* nnQueueGet(int node_num);
	void nnQueuePut(StateVector* state_vector, int node_num);
	ActionDistribution* adQueueGet(int node_num);
	void adQueuePut(ActionDistribution* ad, int node_num);

	void incrementNumQueueSubmissions(int minibatch_num);
	int numQueueSubmissions(int minibatch_num);
	int totalNumQueueSubmissions();
	void logNumQueueSubmissions();
	void logQueue();

	// promising

	/* Called by Worker Thread. Causes the current worker thread to wait until it is safe to process this specific node.
	 * This function first makes sure the minibatch is available, then checks that this node has not already been processed by this thread
	 * since its minibatch was last freshened by the Master. */
	void workerWaitForNode(int node_num, int thread_num); 

		/* Marks that this thread has processed this node.  This prevents the looping problem. */
	void markNodeProcessed(int node_num, int thread_num);


private:

	

	/* Called by worker thread. Marks the given minibatch safe for the master to consume. */
	void markMasterSafe(int minibatch_num, bool from_locked=true);

	/* Called by master thread. Marks the given minibatch safe for workers to touch. */
	void markWorkerSafe(int minibatch_num);

	/* Throws error if this is not a valid thread number. */
	void assertValidThreadNum(int thread_num);

	/* Throws error if this is not a valid minibatch number. */
	void assertValidMinibatchNum(int minibatch_num);

	/* Throws error if this is not a valid node number. */
	void assertValidNodeNum(int node_num);


	/* Decrements the counter for the number of nodes left to submit for this minibatch.
	 * If at 0, and we are trying to decrement, throws an error.
	 * If the number of nodes left to submit for this minibatch is now 0, mark the minibatch as master safe, reset the nodes_left_to_submit counter,
	 * and signal the master to wake up.
	 * Returns the new number of nodes left to submit for this minibatch. */
	int decrementNodesLeft(int minibatch_num, int node_num, bool lock_needed=true);

	// experimental
	/* Returns true if this thread has already processed this node since the last freshening by the master. */
	bool threadHasAlreadyProcessed(int node_num, int thread_num);







	int num_nodes;
	int minibatch_size;
	int num_minibatches;
	int num_threads;
	int num_nodes_per_thread;

	vector<MCTS_Node*>* all_nodes; // all the MCTS_Nodes that worker threads process
	vector<StateVector*>* nn_queue; // workers submit to this queue, master consumes from it
	vector<ActionDistribution*>* nn_outputs; // master submits to this queue, workers consume from it

	vector<bool>* minibatch_ownership; // minibatch_ownership[i] is true if minibatch i is safe for master, false if safe for worker
	
	vector<int>* active_nodes_in_minibatch; // i-th entry denotes how many nodes still need work in i-th minibatch
	vector<int>* active_nodes_in_thread; // j-th entry denotes how many of thread j's nodes still need work
	vector<int>* nodes_left_to_submit; // the i-th entry denotes how many nodes still need submission before this minbatch is master safe

	int num_active_minibatches; // the number of minibatches for which there is one or more active node
	int num_active_threads; // the number of minibatches for which there is one or more active node


	vector<int>* current_node_of_thread; // notes the node that each worker thread is currently processing, holds -1 if all the thread's nodes are done
	// these next two vectors implement a doubly linked list of active node indexes
	vector<int>* next_active_node; // the i-th entry notes the index of the first node after the i-th node that is still active
	vector<int>* prev_active_node; // the i-th entry notes the index of the first node before the i-th node that is still active


	int master_current_minibatch; // notes the minibatch on which the master is currently working
	vector<int>* next_active_minibatch; // the i-th entry notes the index of the first minibatch after the i-th minibatch that is still active
	vector<int>* prev_active_minibatch; // the i-th entry notes the index of the first minibatch before the i-th minibatch that is still active

	// workers and masters cannot simultaneously touch the same minibatch
	mutex minibatch_safe_mutex;
	condition_variable minibatch_safe_cv;

	// protect all shared data (flags, counter variables)
	mutex shared_data_mutex;

	// protect the output stream
	mutex cout_mutex;

	// for logging purposes
	map<__thread_id, string>* thread_names;

	// experimental
	mutex queue_mutex;
	vector<int>* num_queue_submissions; // testing only
	int total_num_queue_submissions;

	// experimental with promise
	map<int, vector<int>*>* num_submissions_per_thread; // maps minibatch numbers to vector of size num_threads
	// num_submissions_per_thread[b][t] gives the number of submissions that Thread T has made to minibatch B since batch B was last reset by master
	// this prevents the problem of a single thread looping back before master gets control and continuing to work on a batch it has already seen
	map<int, vector<int>*>* left_to_submit_per_thread;
	// left_to_submit_per_thread[b][t] gives the number of submissions that Thread T has yet to make to minibatch B since minibatch B was last reset by master
	// when a minibatch is marked Worker safe, this number needs to be appropriately reset to the number of active nodes for thread T in minibatch B
	// this prevents the problem of looping back to a node and thinking it's safe becasue other nodes in this minibatch for this thread are permanently finished
	map<int, vector<int>*>* active_nodes_in_minibatch_per_thread;
	// active_nodes_in_minibatch_per_thread[b][t] gives the number of active (not permanently finished) nodes that Thread T controls in minibatch B


	//for testing
	void checkBeforeADEnqueue(int minibatch_num, vector<ActionDistribution*>* ads_to_submit, int round);
	void checkAfterADEnqueue(int minibatch_num, vector<ActionDistribution*>* ads_submitted, int round);

};

#endif