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
	if (num_legal_moves <= 0) {
		cout << "NUm legal moves <= 0" << endl;
		this->printBoard();
		cout << "is terminal? " << this->isTerminalState() << endl;
	}
	ASSERT(num_legal_moves > 0, "No legal moves available from this hex state.");
	
	int r = rand() % num_legal_moves;
	set<int>::const_iterator it(this->_legal_actions.begin());
	advance(it, r);
	return *it;}



vector<int> HexState::board() const {
	return this->_board;
}

void HexState::makeStateVector(vector<double>* state_vector) const {

	ASSERT(state_vector != NULL, "The vector passed to makeStateVector Cannot be null");

	int padded_dim = this->_dimension + 4;
	int row_size = padded_dim, col_size = padded_dim;
	int channel_size = row_size * col_size;

	// initialize with zeros
	for (int i = 0; i < (2 * channel_size) + 2; i++) {
		state_vector->push_back(0.0);
	}


	// pad top and bottom in first channel
	for (int col_num = 0; col_num < col_size; col_num++) {
		int top_row = 0;
		int second_row = 1;
		int second_to_bottom_row = padded_dim - 2;
		int bottom_row = padded_dim - 1;

		state_vector->at((top_row * row_size) + col_num) = 1.0;
		state_vector->at((second_row * row_size) + col_num) = 1.0;
		state_vector->at((second_to_bottom_row * row_size) + col_num) = 1.0;
		state_vector->at((bottom_row * row_size) + col_num) = 1.0;

	}

	// pad left and right in second channel
	for (int row_num = 0; row_num < row_size; row_num++) {
		int left_col = 0;
		int second_col = 1;
		int second_to_right_col = padded_dim - 2;
		int right_col = padded_dim - 1;

		state_vector->at((row_num * row_size) + left_col + channel_size) = 1.0;
		state_vector->at((row_num * row_size) + second_col + channel_size) = 1.0;
		state_vector->at((row_num * row_size) + second_to_right_col + channel_size) = 1.0;
		state_vector->at((row_num * row_size) + right_col + channel_size) = 1.0;
	}

	// place Player stones
	for (int pos = 0; pos < this->numActions(); pos++) {

		int r = this->row(pos) + 2;
		int c = this->col(pos) + 2;

		// Player 1 stone in first channel
		if (this->_board[pos] == 1) {
			state_vector->at((r * row_size) + c) = 1.0;
		}

		// Player 2 stone in second channel
		if (this->_board[pos] == -1) {
			state_vector->at((r * row_size) + c + channel_size) = 1.0;
		}
	}

	// add the 2 bits for the turn
	ASSERT(this->turn() == 1 || this->turn() == -1, "Turn must be 1 or -1");
	if (this->turn() == 1) {
		state_vector->at(2 * channel_size) = 1.0;
	} else {
		state_vector->at(2 * channel_size + 1) = 1.0;
	}

	// make sure correct size
	ASSERT(state_vector->size() == (2 * channel_size) + 2, "Channel size should be " << (2 * channel_size) + 2);

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

vector<int> HexState::neighbors(int pos) const {
	
	int nw = this->northwestNeighbor(pos);
    int ne = this->northeastNeighbor(pos);
    int sw = this->southwestNeighbor(pos);
    int se = this->southeastNeighbor(pos);
    int w = this->westNeighbor(pos);
    int e = this->eastNeighbor(pos);

    vector<int> all_neighbors = {nw, ne, sw, se, w, e};
    vector<int> existing_neighbors;
    for (int neighbor : all_neighbors) {
    	if (neighbor != -1) {
    		existing_neighbors.push_back(neighbor);
    	}
    }

    return existing_neighbors;
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

	vector<int> neighbors = this->neighbors(start_pos);
	for (int neighbor : neighbors) {
		if (!visited[neighbor]) {
			if (this->southwardPathExists(neighbor, end_row, visited)) {
				return true;
			}
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

	vector<int> neighbors = this->neighbors(start_pos);
	for (int neighbor : neighbors) {
		if (!visited[neighbor]) {
			if (this->eastwardPathExists(neighbor, end_col, visited)) {
				return true;
			}
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

string HexState::pieceAsString(int piece) const {
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
		cout << this->pieceAsString(this->at(pos(row, col))) << "    ";
	}
	cout << endl;
}

void HexState::printBoard() const {

	for (int row = 0; row < this->_dimension; row++) {
		this->printRow(row);
	}

}

string HexState::asCSVString() const {

	// make state vector
	vector<double>* sv = new vector<double>();
	this->makeStateVector(sv);

	string s = "";
	for (int pos = 0; pos < sv->size(); pos++) {
		s += to_string(sv->at(pos));
		if (pos < sv->size() - 1) {
			s += ",";
		} 
	}

	delete sv;
	return s;
}







