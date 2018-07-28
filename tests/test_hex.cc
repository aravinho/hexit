#include <iostream>
#include <map>

#include "test_hex.h"

using namespace std;

int test_dimension = 3;
map<string, HexState*> boards = 
	{	
		{"empty", new HexState(test_dimension, {0,0,0,0,0,0,0,0,0}, "basic")},
	 	{"simple_draw", new HexState(test_dimension, {1,0,0,0,-1,0,0,0,1}, "win_fast")},
	 	{"complex_draw", new HexState(test_dimension, {1,1,-1,1,-1,-1,0,1,-1}, "basic")},

	 	{"white_move_4", new HexState(test_dimension, {0,0,0,0,1,0,0,0,0}, "basic")},
	 	{"black_move_3", new HexState(test_dimension, {1,0,0,-1,-1,0,0,0,1}, "win_fast")},
	 	{"white_move_6", new HexState(test_dimension, {1,1,-1,1,-1,-1,1,1,-1}, "basic")},

	 	{"simple_p1_win", new HexState(test_dimension, {0,0,1,-1,1,0,0,1,-1}, "win_fast")},
	 	{"complex_p1_win", new HexState(test_dimension, {1,1,-1,-1,1,1,-1,-1,1}, "basic")},
	 	{"simple_p2_win", new HexState(test_dimension, {0,0,1,0,1,-1,-1,-1,1}, "win_fast")},
	 	{"complex_p2_win", new HexState(test_dimension, {-1,1,-1,-1,-1,1,1,1,1}, "basic")}
	};



void testWinnerHex() {

	// test empty
	ASSERT(boards["empty"]->winner() == 0, "winner should be 0");

	// test basic early game
	ASSERT(boards["simple_draw"]->winner() == 0, "winner should be 0");

	// test complex non-win
	ASSERT(boards["complex_draw"]->winner() == 0, "winner should be 0");

	// test basic Player 1 win
	ASSERT(boards["simple_p1_win"]->winner() == 1, "winner should be 1");

	// test complex Player 1 win
	ASSERT(boards["complex_p1_win"]->winner() == 1, "Winner should be 1");

	// test basic Player 2 win
	ASSERT(boards["simple_p2_win"]->winner() == -1, "Winner should be -1");

	// test complex Player 2 win
	ASSERT(boards["complex_p2_win"]->winner() == -1, "Winner should be -1");
}


void testIsTerminalStateHex() {
	// test empty
	ASSERT(!boards["empty"]->isTerminalState(), "should be nonterminal");

	// test basic early game
	ASSERT(!boards["simple_draw"]->isTerminalState(), "should be nonterminal");

	// test complex non-win
	ASSERT(!boards["complex_draw"]->isTerminalState(), "should be nonterminal");

	// test basic Player 1 win
	ASSERT(boards["simple_p1_win"]->isTerminalState(), "should be terminal");

	// test complex Player 1 win
	ASSERT(boards["complex_p1_win"]->isTerminalState(), "should be terminal");

	// test basic Player 2 win
	ASSERT(boards["simple_p2_win"]->isTerminalState(), "should be terminal");

	// test complex Player 2 win
	ASSERT(boards["complex_p2_win"]->isTerminalState(), "should be terminal");
}


void testRewardHex() {	
	// test empty
	ASSERT(boards["empty"]->reward() == 0, "Reward should be 0");

	// test basic early game
	ASSERT(boards["simple_draw"]->reward() == 0, "Reward should be 0");

	// test complex non-win
	ASSERT(boards["complex_draw"]->reward() == 0, "Reward should be 0");

	// test basic Player 1 win
	ASSERT(boards["simple_p1_win"]->reward() == 5, "Reward should be 5");

	// test complex Player 1 win
	ASSERT(boards["complex_p1_win"]->reward() == 1, "Reward should be 1");

	// test basic Player 2 win
	ASSERT(boards["simple_p2_win"]->reward() == -4, "Reward should be -4");

	// test complex Player 2 win
	ASSERT(boards["complex_p2_win"]->reward() == -1, "Reward should be -1");
}


void testTurnHex() {
	// test empty
	ASSERT(boards["empty"]->turn() == 1, "Turn should be 1");

	// test basic early game
	ASSERT(boards["simple_draw"]->turn() == -1, "Turn should be -1");

	// test complex non-win
	ASSERT(boards["complex_draw"]->turn() == 1, "Turn should be 1");
	
}


void testNextStateHex() {

	// test that subtyping of return types works
	EnvState* s = new HexState(test_dimension, {0,0,0,0,0,0,0,0,0}, "basic");
	EnvState* s2 = s->nextState(4);

	// test that next state produces the correct board
	ASSERT(boards["empty"]->nextState(4)->equals(*(boards["white_move_4"])), "White move 4");
	ASSERT(boards["simple_draw"]->nextState(3)->equals(*(boards["black_move_3"])), "Black move 3");
	ASSERT(boards["complex_draw"]->nextState(6)->equals(*(boards["white_move_6"])), "White move 6");
	
}


void testIsLegalActionHex() {

	// all moves legal from start, none legal from terminal state
	ASSERT(boards["empty"]->isLegalAction(5), "Any move legal at first");
	ASSERT(boards["empty"]->isLegalAction(8), "Any move legal at first");
	ASSERT(!boards["simple_p1_win"]->isLegalAction(0), "No move legal from terminal state");
	ASSERT(!boards["complex_p2_win"]->isLegalAction(7), "No move legal from terminal state");

	ASSERT(boards["simple_draw"]->isLegalAction(2), "Legal action 2");
	ASSERT(!boards["simple_draw"]->isLegalAction(4), "Illegal action 4");
	ASSERT(boards["complex_draw"]->isLegalAction(6), "Legal action 6");
	ASSERT(!boards["complex_draw"]->isLegalAction(3), "Illegal action 3");
	
}


void testRandomActionHex() {

	set<int> a_set = {0,1,2,3,4,5,6,7,8};
	set<int> b_set = {1,2,3,5,6,7};
	set<int> c_set = {6};

	// try several random action samples and make sure they are in the correct range
	for (int t = 0; t < 10; t++) {

		int a = boards["empty"]->randomAction();
		int b = boards["simple_draw"]->randomAction();
		int c = boards["complex_draw"]->randomAction();

		ASSERT(a_set.count(a) > 0, "Random move for empty");
		ASSERT(b_set.count(b) > 0, "Random move for simple draw");
		ASSERT(c_set.count(c) > 0, "Random move for complex draw");
	}

	
}


void testAsCSVStringHex() {
	ASSERT(boards["empty"]->asCSVString() == "0,0,0,0,0,0,0,0,0", "Empty board CSV String incorrect");
	ASSERT(boards["complex_p2_win"]->asCSVString() == "-1,1,-1,-1,-1,1,1,1,1", "CSV String incorrect for complex p2 win");

}

void testPrintBoardHex() {
	cout << "printing sample board {0,0,1,0,1,-1,-1,-1,1}" << endl;
	boards["simple_p2_win"]->printBoard();
}






void runHexTests() {
	cout << "Running Hex Tests..." << endl << endl;
	testWinnerHex();
	testIsTerminalStateHex();
	testRewardHex();
	testTurnHex();
	testNextStateHex();
	testIsLegalActionHex();
	testRandomActionHex();
	testAsCSVStringHex();
	testPrintBoardHex();
	cout << "Finished running Hex Tests." << endl << endl;
}


