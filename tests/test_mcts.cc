#include <iostream>
#include <vector>
#include <set>
#include <stdlib.h> // srand, rand
#include <time.h> // to seed RNG


#include "test_mcts.h"
#include "../src/tictactoe.h"
#include "../src/mcts.h"
#include "test_utils.h"







void runMctsTests() {
	// seed RNG
	srand(time(NULL));
	runPythonScript();
	//cout << "Testing MCTS..." << endl << endl;
	//cout << "Done Testing MCTS." << endl << endl;

}
