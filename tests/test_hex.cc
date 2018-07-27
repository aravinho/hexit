#include <iostream>

#include "test_hex.h"

using namespace std;

void testHexWinner() {
	int dimension = 5;
	vector<int> board(dimension * dimension, 0);
	EnvState* hex_board = new HexState(dimension, board);
	ASSERT(hex_board->winner() == 0, "Winner should initially be 0");

}

void runHexTests() {
	cout << "Running Hex Tests..." << endl << endl;
	testHexWinner();
	cout << "Finished running Hex Tests." << endl << endl;
}


