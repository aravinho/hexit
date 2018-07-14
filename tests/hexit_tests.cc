#include<iostream> 
#include "test_tictactoe.h"
#include "test_mcts.h"

using namespace std;
 
int main()
{
	cout << endl << "Beginning Tests..." << endl << endl;
	runTictactoeTests();
	//runMctsTests();
	runThreadManagerTests();
	cout << endl << "Finished Tests..." << endl << endl;
}

