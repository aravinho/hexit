#ifndef MCTS_H

#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>


using namespace std;


void runPythonScript(string filename);

class MCTS_Node {
public:
	int x;
	MCTS_Node(int x);
	MCTS_Node();
	MCTS_Node* setX(int x);
};

class StateVector {
public:
	int y;
	StateVector(int y);
};

class ActionDistribution {
public:
	int z;
	ActionDistribution(int z);
};

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


/*int DEFAULT_NUM_NODES = 64;
int DEFAULT_MINIBATCH_SIZE = 16;
int DEFAULT_num_threads = 4;*/

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

	/* Called by Worker Thread. Causes the current worker thread to wait until it is safe to process nodes in this minibatch.
	 * Returns true if the minibatch is permanently complete, and false if there are still nodes that need work. */ 
	bool workerWaitForMinibatch(int minibatch_num);

	/* Called by Master Thread. Causes the master thread to wait until either:
	* - it is safe for the master to process nodes in this minibatch, OR
	* - the minibatch is permanently completed.
	* Returns true if the minibatch is permenantly complete, and false if there are still nodes that need work. */
	void masterWaitForMinibatch(int minibatch_num);

	/* Called by Worker Thread.  Submits the given state_vector, which corresponds to the node indexed by node_num.
	 * Places this state_vector into the nn_queue, and increments the ready-counters for this minibatch.
	 * If the submission of this state vector causes its minibatch to become NN-ready, sets the appropriate flags,
	 * notifies the master thread, and then resets the appropriate counter variables to prepare for the next round of processing. */
	void submitToNNQueue(const StateVector& state_vector, int node_num);

	/* Called by worker thread.  Returns a pointer to the node indexed by node_num. */
	MCTS_Node* getNode(int node_num);

	/* Called by Master thread, once the NN results for the given minibatch are safely written to the nn_outputs vector.
	 * This resets appropriate flags, releases the master's hold on this minibatch, and signals to workers that there is a fresh minibatch for them to work on. */
	void masterDoneWithMinibatch(int minibatch_num);

	/* Called by Worker thread, when a node has permanently finished its computation.
	 * Decrements the active-node counters for this node's minibatch and thread. */
	void nodeComplete();

	/* Just for testing. */
	void flipOwnershipFlag(int minibatch_num);

private:

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

	// workers and masters cannot simultaneously touch the same minibatch
	mutex m_minibatch_safe;
	condition_variable cv_minibatch_safe;

	// protect all shared data (flags, counter variables)
	mutex m_shared_data;


};


#endif