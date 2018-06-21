#include <iostream>
#include <vector>
#include <set>
#include <stdlib.h> // srand, rand
#include <time.h> // to seed RNG


#include "test_tictactoe.h"
#include "../src/tictactoe.h"
#include "test_utils.h"

vector<int> e = {0,0,0,0,0,0,0,0,0};
vector<int> p1ws = {1,0,0,1,-1,0,1,0,-1};
vector<int> p2ws = {1,0,1,-1,-1,-1,0,1,0};
vector<int> p1wc = {1,-1,1,-1,1,-1,1,-1,1};
vector<int> p2wc = {-1,0,1,0,-1,1,1,0,-1};
vector<int> snw = {1,0,0,0,-1,0,0,0,1};
vector<int> cnw = {1,0,1,-1,-1,1,1,-1,-1};
vector<int> fm7 = {0,0,0,0,0,0,0,1,0};
vector<int> bm3 = {1,0,0,-1,-1,0,0,0,1};
vector<int> wm1 = {1,1,1,-1,-1,1,1,-1,-1};

Tictactoe emp;
Tictactoe p1win_simple(p1ws);
Tictactoe p2win_simple(p2ws);
Tictactoe p1win_complex(p1wc);
Tictactoe p2win_complex(p2wc);
Tictactoe simple_nowin(snw);
Tictactoe complex_nowin(cnw);
Tictactoe first_move7(fm7);
Tictactoe black_move3(bm3);
Tictactoe white_move1(wm1);

void assertTrue(bool cond, string test_name, string message) {
	string test_file = "test_tictactoe.cpp";
	assertWithMessage(cond, test_file, test_name, message);
}

void testWinner() {
	assertTrue(emp.winner() == 0, "testWinner", "Empty has no winner");
	assertTrue(p1win_simple.winner() == 1, "testWinner", "P1 Simple Win");
	assertTrue(p2win_simple.winner() == -1, "testWinner", "P2 Simple Win");
	assertTrue(p1win_complex.winner() == 1, "testWinner", "P1 Complex Win");
	assertTrue(p2win_complex.winner() == -1, "testWinner", "P2 Complex Win");
	assertTrue(simple_nowin.winner() == 0, "testWinner", "Simple no win");
	assertTrue(complex_nowin.winner() == 0, "testWinner", "Complex no win");
}

void testIsTerminalState() {
	assertTrue(emp.winner() == 0, "testisTerminalState", "Empty has no winner");
	assertTrue(p1win_simple.isTerminalState(), "testIsTerminalState", "P1 Simple Win");
	assertTrue(p2win_simple.isTerminalState(), "testIsTerminalState", "P2 Simple Win");
	assertTrue(p1win_complex.isTerminalState(), "testIsTerminalState", "P1 Complex Win");
	assertTrue(p2win_complex.isTerminalState(), "testIsTerminalState", "P2 Complex Win");
	assertTrue(!simple_nowin.isTerminalState(), "testIsTerminalState", "Simple no win");
	assertTrue(!complex_nowin.isTerminalState(), "testIsTerminalState", "Complex no win");
}

void testReward() {	
	assertTrue(emp.reward() == 0, "testReward", "Empty has no winner");
	assertTrue(p1win_simple.reward() == 1, "testReward", "P1 Simple Win");
	assertTrue(p2win_simple.reward() == -1, "testReward", "P2 Simple Win");
	assertTrue(p1win_complex.reward() == 1, "testReward", "P1 Complex Win");
	assertTrue(p2win_complex.reward() == -1, "testReward", "P2 Complex Win");
	assertTrue(simple_nowin.reward() == 0, "testReward", "Simple no win");
	assertTrue(complex_nowin.reward() == 0, "testReward", "Complex no win");
}

void testNextState() {
	assertTrue(emp.nextState(7) == first_move7, "testNextState", "First move");
	assertTrue(simple_nowin.nextState(3) == black_move3, "testNextState", "Black Move Square 3");
	assertTrue(complex_nowin.nextState(1) == white_move1, "testNextState", "White Move Square 1");
}

void testIsLegalAction() {
	assertTrue(emp.isLegalAction(5), "testIsLegalAction", "Any move legal at first");
	assertTrue(emp.isLegalAction(8), "testIsLegalAction", "Any move legal at first");
	assertTrue(!p1win_simple.isLegalAction(0), "testIsLegalAction", "No legal moves from terminal state");
	assertTrue(!p2win_complex.isLegalAction(4), "testIsLegalAction", "No legal moves from terminal state");
	assertTrue(simple_nowin.isLegalAction(2), "testIsLegalAction", "Black 2 is legal");
	assertTrue(!simple_nowin.isLegalAction(4), "testIsLegalAction", "Black 4 is illegal");
	assertTrue(complex_nowin.isLegalAction(1), "testIsLegalAction", "White 1 is legal");
	assertTrue(!complex_nowin.isLegalAction(7), "testIsLegalAction", "White 7 is illegal");
}

void testRandomAction() {
	int a = emp.randomAction();
	int b = simple_nowin.randomAction();
	int c = complex_nowin.randomAction();

	set<int> a_set = {0,1,2,3,4,5,6,7,8};
	set<int> b_set = {1,2,3,5,6,7};
	set<int> c_set = {1};

	assertTrue(a_set.count(a) > 0, "testRandomAction", "Any move possible at first");
	assertTrue(b_set.count(b) > 0, "testRandomAction", "Only certain possible moves");
	assertTrue(c_set.count(c) > 0, "testRandomAction", "Only one move possible");
}

void testTurn() {
	assertTrue(emp.turn() == 1, "testTurn", "White's move to start");
	assertTrue(simple_nowin.turn() == -1, "testTurn", "Black's move here");
	assertTrue(complex_nowin.turn() == 1, "testTurn", "White's move here");
}





void runTictactoeTests() {
	// seed RNG
	srand(time(NULL));


	testWinner();
	testIsTerminalState();
	testReward();
	testNextState();
	testIsLegalAction();
	testRandomAction();
	testTurn();
}
