#include "hex_state.h"
#include "utils.h"

#include <math.h>
#include <iostream>
#include <numeric>
#include <stdlib.h> // srand, rand


using namespace std;


HexState::HexState(int dimension, vector<int> board, string reward_type) {
	this->_board = board;
	this->_dimension = dimension;
	this->_reward_type = reward_type;


	// determine whose turn
	int sum_pieces = accumulate(this->_board.begin(), this->_board.end(), 0);
	ASSERT(sum_pieces == 0 || sum_pieces == 1, "incorrect sum_pieces");
	if (sum_pieces == 0) {
		this->_turn = 1;
	} else if (sum_pieces == 1) {
		this->_turn = -1;
	}

	// determine winner
	this->_winner = this->determineWinner();
	this->_is_terminal = (this->_winner != 0);

	// determine legal actions
	int num_total_actions = pow(dimension, 2);
	if (!this->_is_terminal) {
		for (int pos = 0; pos < num_total_actions; pos++) {
			if (board[pos] == 0) {
				this->_legal_actions.insert(pos);
			}
		}
	}

}

HexState::~HexState() {

}

int HexState::winner() const {
	return _winner;
}

bool HexState::isTerminalState() const {
	return this->_is_terminal;
}

double HexState::reward() const {

	if (!this->isTerminalState()) {
		return 0.0;
	}

	if (this->_reward_type == "basic") {
		return (double) this->_winner;
	}

	if (this->_reward_type == "win_fast") {
		int num_pieces_played = 0;
		for (int piece : this->board()) {
			if (piece != 0) {
				num_pieces_played += 1;
			}
		}
		return (double) (this->_winner * ((this->numActions() + 1) - num_pieces_played));
	}

	return 0.0;
}

int HexState::turn() const {
	return this->_turn;
}

HexState* HexState::nextState(int action) {

	ASSERT(this->isLegalAction(action), "illegal action " << action << " in nextState function for HexState");

	if (this->isTerminalState()) {
		return this;
	}

	ASSERT(this->at(action) == 0, "Cannot make action " << action << " since it is already made in this Hex game");
	vector<int> new_board(this->board());
	new_board[action] = this->turn();
	return new HexState(this->_dimension, new_board, this->_reward_type);
}

bool HexState::equals(const EnvState& other) const {

	vector<int> this_board = this->board();
	vector<int> other_board = other.board();
	if (this_board.size() != other_board.size()) {
		return false;
	}
	for (int pos = 0; pos < this_board.size(); pos++) {
		if (this_board[pos] != other_board[pos]) {
			return false;
		}
	}
	return true;
}

bool HexState::isLegalAction(int action) const {
	return this->_legal_actions.count(action) > 0;
}

int HexState::randomAction() const {
	int num_legal_moves = this->_legal_actions.size();
	ASSERT(num_legal_moves > 0, "No legal moves available from this hex state.");
	
	int r = rand() % num_legal_moves;
	set<int>::const_iterator it(this->_legal_actions.begin());
	advance(it, r);
	return *it;}



vector<int> HexState::board() const {
	return this->_board;
}

vector<double>* HexState::makeStateVector() const {

	// for now, just the board
	vector<double>* state_vector = new vector<double>(this->numActions());
	for (int pos = 0; pos < this->numActions(); pos++) {
		state_vector->at(pos) = (double) this->_board[pos];
	}
	return state_vector;
}

int HexState::numActions() const {
	return pow(this->_dimension, 2);
}





/***** Private helper functions *****/


int HexState::at(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to at function");
	return this->_board[pos];

}

int HexState::row(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to row function");
	return pos / this->_dimension;
}

int HexState::col(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to col function");
	return pos % this->_dimension;
}

int HexState::pos(int row, int col) const {
	ASSERT(0 <= row && row < this->_dimension, "Illegal row argument to pos in HexState");
	ASSERT(0 <= col && col < this->_dimension, "Illegal col argument to pos in HexState");
	return (row * this->_dimension) + col;
}

int HexState::northwestNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to northwestNeighbor function");
	int r = this->row(pos); int c = this->col(pos);
	if (r == 0) {
		return -1;
	}

	return this->pos(r - 1, c);
}

int HexState::northeastNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to northeastNeighbor function");
	
	int r = this->row(pos); int c = this->col(pos);
	if (r == 0 || c == this->_dimension - 1) {
		return -1;
	}

	return this->pos(r - 1, c + 1);

}

int HexState::southwestNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to southwestNeighbor function");
	int r = this->row(pos); int c = this->col(pos);
	if (r == this->_dimension - 1|| c == 0) {
		return -1;
	}

	return this->pos(r + 1, c - 1);

}

int HexState::southeastNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to southeastNeighbor function");
	int r = this->row(pos); int c = this->col(pos);
	if (r == this->_dimension - 1) {
		return -1;
	}

	return this->pos(r + 1, c);

}

int HexState::westNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to westNeighbor function");
	int r = this->row(pos); int c = this->col(pos);
	if (c == 0) {
		return -1;
	}

	return this->pos(r, c - 1);

}

int HexState::eastNeighbor(int pos) const {
	ASSERT(0 <= pos && pos < this->numActions(), "Illegal position argument to eastNeighbor function");
	int r = this->row(pos); int c = this->col(pos);
	if (c == this->_dimension - 1) {
		return -1;
	}

	return this->pos(r, c + 1);


}


bool HexState::southwardPathExists(int start_pos, int end_row, vector<bool>& visited) const {
	
	ASSERT(0 <= start_pos && start_pos < this->numActions(), "Illegal start position argument to southwardPathExists function");
	ASSERT(0 <= end_row && end_row < this->_dimension, "Illegal end row argument to southwardPathExists function");
	ASSERT(visited.size() == this->numActions(), "Visited bitvector is incorrect size in southwardPathExists function");

	// this spot must be occupied by Player 1
	if (this->at(start_pos) != 1) {
		return false;
	}

	// if we are in the end row, return true
	if (this->row(start_pos) == end_row) {
		return true;
	}

	// mark this spot visited
	visited[start_pos] = true;

	int southwest_neighbor = this->southwestNeighbor(start_pos);
	int southeast_neighbor = this->southeastNeighbor(start_pos);
	int east_neighbor = this->eastNeighbor(start_pos);

	// recurse on southwest neighbor
	if (southwest_neighbor != -1 && !visited[southwest_neighbor]) {
		if (this->southwardPathExists(southwest_neighbor, end_row, visited)) {
			return true;
		}
	}

	// recurse on southeast neighbor
	if (southeast_neighbor != -1 && !visited[southeast_neighbor]) {
		if (this->southwardPathExists(southeast_neighbor, end_row, visited)) {
			return true;
		}
	}

	// recurse on east neighbor
	if (east_neighbor != -1 && !visited[east_neighbor]) {
		if (this->southwardPathExists(east_neighbor, end_row, visited)) {
			return true;
		}
	}

	return false;

}

bool HexState::eastwardPathExists(int start_pos, int end_col, vector<bool>& visited) const {

	ASSERT(0 <= start_pos && start_pos < this->numActions(), "Illegal start position argument to eastwardPathExists function");
	ASSERT(0 <= end_col && end_col < this->_dimension, "Illegal end col argument to eastwardPathExists function");
	ASSERT(visited.size() == this->numActions(), "Visited bitvector is incorrect size in eastwardPathExists function");

	// this spot must be occupied by Player 2
	if (this->at(start_pos) != -1) {
		return false;
	}

	// if we are in the end col, return true
	if (this->col(start_pos) == end_col) {
		return true;
	}

	// mark this spot visited
	visited[start_pos] = true;

	int east_neighbor = this->eastNeighbor(start_pos);
	int northeast_neighbor = this->northeastNeighbor(start_pos);
	int southeast_neighbor = this->southeastNeighbor(start_pos);

	// recurse on east neighbor
	if (east_neighbor != -1 && !visited[east_neighbor]) {
		if (this->eastwardPathExists(east_neighbor, end_col, visited)) {
			return true;
		}
	}

	// recurse on northeast neighbor
	if (northeast_neighbor != -1 && !visited[northeast_neighbor]) {
		if (this->eastwardPathExists(northeast_neighbor, end_col, visited)) {
			return true;
		}
	}


	// recurse on southeast neighbor
	if (southeast_neighbor != -1 && !visited[southeast_neighbor]) {
		if (this->eastwardPathExists(southeast_neighbor, end_col, visited)) {
			return true;
		}
	}

	return false;

}

bool HexState::northSouthPathExists() const {
	int end_row = this->_dimension - 1;

	// look for southward paths starting at all the positions on the top row
	for (int pos = 0; pos < this->_dimension; pos++) {
		vector<bool> visited(this->numActions(), false);
		if (this->southwardPathExists(pos, end_row, visited)) {
			return true;
		}
	}

	return false;
}

bool HexState::westEastPathExists() const {
	int end_col = this->_dimension - 1;

	// look for eastward paths starting at all the positions on the left col
	for (int pos = 0; pos < this->numActions(); pos += this->_dimension) {
		vector<bool> visited(this->numActions(), false);
		if (this->eastwardPathExists(pos, end_col, visited)) {
			return true;
		}
	}

	return false;
}


int HexState::determineWinner() const {
	// check if there is a North to South path
	if (this->northSouthPathExists()) {
		return 1;
	}

	// check if there is a West to East path
	if (this->westEastPathExists()) {
		return -1;
	}

	return 0;

}




/***** Utility Functions *****/

string pieceAsString(int piece) {
	if (piece == 1) return "X";
	if (piece == -1) return "O";
	return "_";
}

void HexState::printRow(int row) const {

	// print offset
	for (int off = 0; off < row; off++) {
		cout << "  ";
	}

	for (int col = 0; col < this->_dimension; col++) {
		cout << pieceAsString(this->at(pos(row, col))) << "    ";
	}
	cout << endl;
}

void HexState::printBoard() const {

	for (int row = 0; row < this->_dimension; row++) {
		this->printRow(row);
	}

}

string HexState::asCSVString() const {
	string s = "";
	for (int pos = 0; pos < this->numActions(); pos++) {
		s += to_string(at(pos));
		if (pos < this->numActions() - 1) {
			s += ",";
		} 
	}
	return s;
}







