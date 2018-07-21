#include "main.h"
#include <iostream>
#include "stdlib.h"
#include <ctime>
using namespace std;


void generateRandomDataBatch(int batch_size, string base_filename) {

	int num_simulations = 1000;
	int max_depth = 5;

	// create dummy ad
	vector<int>* dummy_ad_vec = new vector<int>(9, 0);
	ActionDistribution* dummy_ad = new ActionDistribution(dummy_ad_vec);

	vector<MCTS_Node*>* completed_nodes = new vector<MCTS_Node*>(batch_size);
	for (int i = 0; i < batch_size; i++) {
		Tictactoe* board = generateRandomTictactoeBoard();
		if (i % 100 == 0) {
			cout << "running simulations for batch " << i << endl;
		}
		MCTS_Node* node = (new MCTS_Node(board, true, num_simulations))->sampleActions(true);
		node = runAllSimulations(node, dummy_ad, max_depth);
		completed_nodes->at(i) = node;
	}

	writeBatchToFile(completed_nodes, base_filename);
}

int main() {
	cout << "hi main" << endl;
	srand(time(NULL));
	int num_states = 1024;
	generateRandomDataBatch(1024, "data/in/train/random_batch_1024");
}