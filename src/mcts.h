#ifndef MCTS_H

//#include "mcts_node.h"
//#include "mcts_shared_data.h"
#include <vector>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>


using namespace std;


void runPythonScript(string filename);









// just here while testing
void masterFunc(string infile, int num_nodes=64, int minibatch_size=16, int num_threads = 4);


#endif