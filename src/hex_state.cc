#include "hex_state.h"

#include <iostream>

using namespace std;


HexState::HexState(int dimension, vector<int> board) {
	this->board = board;
}

int HexState::winner() {
	return 0;
}

