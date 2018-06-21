#include <numeric>
#include <math.h> // pow
#include <stdlib.h> // srand, rand
#include <iostream>

#include "tictactoe.h"

using namespace std;

// global constants
int NUM_ACTIONS = 9;


int processRow(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions) {
	
	// a base-3 number indicating the make-up of a row/col/diag (which permutation)
	// perm = 9*a_1 + 3*a_2 + 1*a_3, where a_i is the i-th element 
	int perm = 0;
	int piece = 0; // the i-j piece (1, 0 or -1)
	int turn = 1; // 1 if X's turn, -1 if O's

	for (int i = 0; i < 3; i++) {
		
		perm = 0;

		for (int j = 0; j < 3; j++) {
			piece = squares[3*i+j];
			perm += pow(3, j) * piece;
			if (piece == 0) {
				legal_actions.insert(3*i+j);
			} else {
				turn *= -1;
			}
		}

		// check if this row has been won
		if (perm == 13) {
			return 1;
		}
		if (perm == -13) {
			return -1;
		}
		
		// compute possible winning moves
		if (perm == 12 * turn) {
			winning_actions.insert(3*i + 2);
		}

		if (perm == 10 * turn) {	
			winning_actions.insert(3*i + 1);
		}

		if (perm == 4 * turn) {
			winning_actions.insert(3*i + 0);
		} 

	}

	return 0;
}



int processCol(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions) {
	
	// a base-3 number indicating the make-up of a row/col/diag (which permutation)
	// perm = 9*a_1 + 3*a_2 + 1*a_3, where a_i is the i-th element 
	int perm = 0;
	int piece = 0; // the i-j piece (1, 0 or -1)
	int turn = 1; // 1 if X's turn, -1 if O's


	for (int j = 0; j < 3; j++) {
		
		perm = 0;

		for (int i = 0; i < 3; i++) {
			piece = squares[3*i+j];
			perm += pow(3, i) * piece;
			if (piece == 0) {
				legal_actions.insert(3*i+j);
			} else {
				turn *= -1;
			}
		}

		// check if this col has been won
		if (perm == 13) {
			return 1;
		}
		if (perm == -13) {
			return -1;
		}
		
		// compute possible winning moves
		if (perm == 12 * turn) {
			winning_actions.insert(3*2 + j);
		}

		if (perm == 10 * turn) {	
			winning_actions.insert(3*1 + j);
		}

		if (perm == 4 * turn) {
			winning_actions.insert(3*0 + j);
		} 

	}

	return 0;
}


int processMajorDiag(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions) {

	vector<int> diag = {0,4,8};
	
	// a base-3 number indicating the make-up of a row/col/diag (which permutation)
	// perm = 9*a_1 + 3*a_2 + 1*a_3, where a_i is the i-th element 
	int perm = 0;
	int piece = 0;
	int turn = 1; // 1 if X's turn, -1 if O's

	for (int i = 0; i < 3; i++) {
		piece = squares[diag[i]];
		perm += pow(3, i) * piece;
		if (piece == 0) {
			legal_actions.insert(diag[i]);
		} else {
				turn *= -1;
			}

		// check if this diag has been won
		if (perm == 13) {
			return 1;
		}
		if (perm == -13) {
			return -1;
		}

		// compute possible winning moves
		if (perm == 12 * turn) {
			winning_actions.insert(diag[2]);
		}
		if (perm == 10 * turn) {
			winning_actions.insert(diag[1]);
		}
		if (perm == 4 * turn) {
			winning_actions.insert(diag[2]);
		}
		
	}

	return 0;
}


int processMinorDiag(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions) {
	
	vector<int> diag = {2,4,6};

	// a base-3 number indicating the make-up of a row/col/diag (which permutation)
	// perm = 9*a_1 + 3*a_2 + 1*a_3, where a_i is the i-th element 
	int perm = 0;
	int piece = 0;
	int turn = 1; // 1 if X's turn, -1 if O's

	for (int i = 0; i < 3; i++) {
		piece = squares[diag[i]];
		perm += pow(3, i) * piece;
		if (piece == 0) {
			legal_actions.insert(diag[i]);
		} else {
				turn *= -1;
			}

		// check if this diag has been won
		if (perm == 13) {
			return 1;
		}
		if (perm == -13) {
			return -1;
		}

		// compute possible winning moves
		if (perm == 12 * turn) {
			winning_actions.insert(diag[2]);
		}
		if (perm == 10 * turn) {
			winning_actions.insert(diag[1]);
		}
		if (perm == 4 * turn) {
			winning_actions.insert(diag[2]);
		}
		
	}

	return 0;
}



Tictactoe::Tictactoe(vector<int> squares) {
	
	int turn = 0;
	int is_terminal = false;
	int reward = 0;
	set<int> legal_actions;
	set<int> winning_actions;
	
	// determine whose turn
	int sum = std::accumulate(squares.begin(), squares.end(), 0);
	if (sum == 0) {
		turn  = 1;
	} else if (sum == 1) {
		turn = -1;
	} else {
		printf("Illegal board found in constructor.\n");
		exit(1);
	}

	// check rows, cols and diags to see if this state is terminal
	// and to collect legal moves and winning moves

	reward = processRow(squares, legal_actions, winning_actions);
	if (reward == 0) {		
		reward = processCol(squares, legal_actions, winning_actions);
	} if (reward == 0) {
		reward = processMajorDiag(squares, legal_actions, winning_actions);
	} if (reward == 0) {
		reward = processMinorDiag(squares, legal_actions, winning_actions);
	}

	_sq = squares;
	_turn = turn;
	_is_terminal = (reward != 0);
	_reward = reward;
	_legal_actions = legal_actions;
	_winning_actions = winning_actions;

}


int Tictactoe::winner() {
	return _reward;
}

bool Tictactoe::isTerminalState() {
	return _is_terminal;
}

int Tictactoe::reward() {
	return _reward;
}

vector<int> Tictactoe::squares() {
	return _sq;
}

bool Tictactoe::operator ==(Tictactoe &other) {
	vector<int> a = squares();
	vector<int> b = other.squares();

	if (a.size() != b.size()) {
		return false;
	}
	for (int i = 0; i < a.size(); i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

Tictactoe Tictactoe::nextState(int action) {

	// If the current board is terminal, just return this object.
	if (_is_terminal) {
		return *this;
	}

	// make sure given action is legal.
	if (_sq[action] != 0) {
		printf("Illegal action in Tictactoe::nextState.\n");
		exit(1);
	}

	// for now, just do the simple version (not totally efficient)
	// change the squares array, and call the Tictactoe constructor
	vector<int> new_sq(_sq);
	new_sq[action] = _turn;
	return Tictactoe(new_sq);

	/*// switch whose turn it is
	int new_turn = -1 * _turn;

	// if this action causes a terminal state,
	// set the _reward and _is_terminal fields of the new state.
	int new_reward = 0;
	bool new_is_terminal = false;
	if (_winning_actions.count(action) > 0) {
		new_reward = _turn;
		new_is_terminal = true;
	}

	// compute the new square vector and the new set of legal moves
	vector<int> new_sq(_sq);
	new_sq[action] = _turn;

	// compute the new set of legal moves
	// if, for some reason, the given action is not legal, error
	set<int> new_legal_actions(_legal_actions);

	if (new_legal_actions.count(action) == 0) {
		printf("Trying to remove an element that is not present from set.\n");
		exit(1);
	}
	new_legal_actions.erase(action);

	Tictactoe new_state = Tictactoe(new_sq, new_turn, new_is_terminal, \
		new_reward, new_legal_actions);
 	
 	return new_state;*/
}

bool Tictactoe::isLegalAction(int action) {
	return _legal_actions.count(action) > 0;
}

int Tictactoe::randomAction() {		
	int num_legal_moves = _legal_actions.size();
	if (num_legal_moves == 0) {
		printf("No legal moves available from this state. \n");
		exit(1);
	}
	int r = rand() % num_legal_moves;
	set<int>::const_iterator it(_legal_actions.begin());
	advance(it, r);
	return *it;
}

int Tictactoe::turn() {
	return _turn;
}
