#ifndef TICTACTOE_H

#include <vector>

using namespace std;

class Tictactoe {
public:
	Tictactoe(vector<int> squares);
	int winner();
	bool isTerminalState();
	int reward();
	bool operator ==(Tictactoe &other);
	Tictactoe nextState(int action);
	bool isLegalAction(int action);
	int randomAction();
	int turn();

private:
	vector<int> _sq;
};

#endif
