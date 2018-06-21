#ifndef TICTACTOE_H

#include <vector>
#include <set>

using namespace std;

class Tictactoe {
public:
	
	/**
	 * Initializes a board with the given vector of squares (which defaults to the empty board).
	 * A 1 is an X, and a -1 is an O.
	 * Set the _turn, _reward and is_terminal fields appropriately.
	 * Populates the _legal_actions and _winning_actions sets appropriately.
	 */ 
	Tictactoe(vector<int> squares={0,0,0,0,0,0,0,0,0});

	/**
	 * Initializes a board with the given data.
	 * The set of winning actions still needs to be computed.
	 */
	Tictactoe(vector<int> squares, int turn, bool is_terminal, int reward, set<int> legal_moves);

	/**
	 * Returns a 0 if the current board has no winner,
	 * 1 if X has won, and -1 if O has won.
	 */ 
	int winner();

	/**
	 * Returns true if the current board is a terminal state,
	 * and false otherwise.
	 */
	bool isTerminalState();
	
	/**
	 * Returns 0 if the current board is in a nonterminal state,
	 * 1 if X has won, and -1 if O has won.
	 */
	int reward();
	
	/**
	 * Return the reward of the board obtained by taking the given action
	 * from the current state.
	 * 0 if a nonterminal state, 1 if X won, or -1 if O won.
	 */
	int reward(int action);

	/**
	 * Returns the _sq vector.  Used for testing mostly.
	 */
	vector<int> squares();

	/**
	 * Returns true if this board and the other board are the same.
	 * Used for testing mostly.
	 */
	bool operator ==(Tictactoe &other);
	
	/**
	 * Returns the state for the board obtained by taking the given action
	 * from the current board.
	 * Errors if the given action is illegal.
	 */
	Tictactoe nextState(int action);
	
	/**
	 * Return true if the given action is legal to take from the current board.
	 */
	bool isLegalAction(int action);
	
	/**
	 * Return an action, chosen uniformly at random from the available legal actions.
	 * If there are no legal actions, error.
	 */
	int randomAction();
	
	/**
	 * Return 1 if it is X's turn in the current board, or -1 if O's turn.
	 * Error if the board is invalid or in a terminal state.
	 */
	int turn();

private:
	
	/** 
	 * Initializes a board with the given vector of squares,
	 * and set the _turn field to the one given. 
 	 */
	Tictactoe(vector<int> squares, int turn);

	vector<int> _sq;
	int _turn;
	bool _is_terminal;
	int _reward;
	set<int> _legal_actions;
	set<int> _winning_actions;
	

};


/**
 * Examine the rows of this board.
 * Return 1, 0, or -1, the reward of the given board.
 * Populate the sets legal_actions, and winning_actions appropriately.
 */
int processRow(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions);


/**
 * Examine the cols of this board.
 * Return 1, 0, or -1, the reward of the given board.
 * Populate the sets legal_actions, and winning_actions appropriately.
 */
int processCol(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions);

/**
 * Examine the major diag (top left to bottom right) of this board.
 * Return 1, 0, or -1, the reward of the given board.
 * Populate the sets legal_actions, and winning_actions appropriately.
 */
int processMajorDiag(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions);

/**
 * Examine the minor diag (top right to bottom left) of this board.
 * Return 1, 0, or -1, the reward of the given board.
 * Populate the sets legal_actions, and winning_actions appropriately.
 */
int processMinorDiag(vector<int> squares, set<int> &legal_actions, set<int> &winning_actions);

#endif
