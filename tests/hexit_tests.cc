#include "test_tictactoe.h"
#include "test_mcts.h"
#include "test_thread_manager.h"
#include "test_hex.h"

#include<iostream> 
#include <stdlib.h> // srand, rand


using namespace std;
 
int main()
{
	srand(time(NULL));
	cout << endl << "Beginning Tests..." << endl << endl;
	//runTictactoeTests();
	//runMctsTests();
	//runThreadManagerTests();
	runHexTests();
	cout << endl << "Finished Tests..." << endl << endl;
}

