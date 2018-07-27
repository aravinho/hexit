#ifndef HEX_STATE_H
#define HEX_STATE_H

#include "env_state.h"
#include <vector>

using namespace std;

class HexState : public EnvState {

public:

	/* Creates a new Hex game state.  The board is of size DIMENSION x DIMENSION, and the stones are specified by the given BOARD vector.
	 * 1 represents a white stone, and -1 represents a black. */
	HexState(int dimension, vector<int> board);

	/* Returns 1 if player 1 has won in this state, and -1 if player 2 has won.  Return 0 if non-terminal state. */
	int winner();

private:
	vector<int> board;
};


#endif