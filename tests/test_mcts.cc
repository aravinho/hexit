#include <iostream>
#include <vector>
#include <set>
#include <stdlib.h> // srand, rand
#include <time.h> // to seed RNG
#include <chrono> // sleep
#include <thread>


#include "test_mcts.h"
#include "../src/tictactoe.h"
#include "../src/mcts.h"
#include "test_utils.h"



void assertTrueMCTS(bool cond, string test_name, string message) {
	string test_file = "test_mcts.cc";
	assertWithMessage(cond, test_file, test_name, message);
}

void simpleFlagFlip(MCTS_Shared_Data* data) {
	// all minibatches should initially be worker safe
	/*for (int minibatch_num = 0; minibatch_num < 4; minibatch_num++) {
		assertTrueMCTS(data->isWorkerSafe(minibatch_num), "sharedDataSimpleSimulation", "all minibatches must initially be worker safe");
		data->flipOwnershipFlag(minibatch_num);
		assertTrueMCTS(data->isMasterSafe(minibatch_num), "sharedDataSimpleSimulation", "all minibatches must now be master safe from worker");
	}*/
}

void simulationStartup() {
	system("exec rm -r data/tmp/*");

}

void simulationVerification(int sim_num) {
	int result;
	result = system("exec diff data/tmp/nodes_0 data/exp_nodes_0");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 0" << endl;
		exit(1);
	} 



	result = system("exec diff data/tmp/nodes_1 data/exp_nodes_1");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 1" << endl;
		exit(1);
	}

	result = system("exec diff data/tmp/nodes_2 data/exp_nodes_2");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 2" << endl;
		exit(1);
	}

	result = system("exec diff data/tmp/nodes_3 data/exp_nodes_3");
	if (result != 0) {
		cout << "Simulation " << sim_num << " unsuccessful on node batch 3" << endl;
		exit(1);
	}

	if (result == 0) {
		cout << "Simulation " << sim_num << " successful" << endl;
	} 
}

void sharedDataSimpleSimulation(int sim_num) {
	simulationStartup();
	int num_nodes = 64, minibatch_size = 16, num_threads = 4;
	int nodes_per_thread = 16, num_minibatches = 4;

	string initial_states_file = "data/initial_states.txt";

	masterFunc(initial_states_file, num_nodes, minibatch_size, num_threads);

	simulationVerification(sim_num);

}




void runMctsTests() {
	// seed RNG
	srand(time(NULL));
	//runPythonScript("data/nums.txt");
	//cout << "Testing MCTS..." << endl << endl;
	//cout << "Done Testing MCTS." << endl << endl;

}

void runMctsSharedDataTests() {
	cout << "Testing MCTS_Shared_Data class..." << endl << endl;
	for (int sim_num = 0; sim_num < 100; sim_num++) {
		sharedDataSimpleSimulation(sim_num);
	}
	cout << endl << "Done testing MCTS_Shared_Data class." << endl << endl;
}
