#include "tictactoe.h"

using namespace std;

Tictactoe::Tictactoe(vector<int> squares) {
	_sq = squares;
}

int Tictactoe::winner() {
	return 0;
}

bool Tictactoe::isTerminalState() {
	return true;
}

int Tictactoe::reward() {
	return 0;
}

bool Tictactoe::operator ==(Tictactoe &other) {
	return false;
}

Tictactoe Tictactoe::nextState(int action) {
	vector<int> v = {0,0,0,0,0,0,0,0,0};
	return Tictactoe(v);
}

bool Tictactoe::isLegalAction(int action) {
	return false;
}

int Tictactoe::randomAction() {
	return 0;
}

int Tictactoe::turn() {
	return 0;
}
