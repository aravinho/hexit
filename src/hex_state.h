#ifndef HEX_STATE_H
#define HEX_STATE_H

#include "env_state.h"
#include <vector>
#include <set>
#include <string>

using namespace std;

class HexState : public EnvState {

public:

	/** Creates a new Hex game state.  The board is of size DIMENSION x DIMENSION, and the stones are specified by the given BOARD vector.
	 * 1 represents a white stone, and -1 represents a black.
	 * Also takes in a string which specifies which reward function to use. 
	 * "basic" means a simple 1/0/-1 reward. "win_fast" means ((num_total_hexagons + 1) - num_hexagons_played) * winner()
	 */
	HexState(int dimension, vector<int> board, string reward_type="win_fast");

	/** 
	 * Destructor. Nothing to delete.
	 */
	~HexState();

	/* Returns the dimension of this board. */
	int dimension() const;

	/**
	 * Returns a 0 if the current board has no winner,
	 * 1 if Player 1 has won, and -1 if Player 2 has won.
	 */ 
	int winner() const;

	/**
	 * Returns true if the current board is a terminal state,
	 * and false otherwise.
	 */
	bool isTerminalState() const;
	
	/**
	 * Returns 0 if the current board is in a nonterminal state,
	 * Positive if Player 1 has won, and negative if Player 2 has won.
	 * The specific internals are specified by the _reward_type field (see constructor comment)
	 */
	double reward() const;

	/**
	 * Max possible reward from this state.
	 */
	double maxReward() const;

	/**
	 * Return 1 if it is Player 1's turn in the current board, or -1 if Player 2's turn.
	 * Error if the board is invalid or in a terminal state.
	 */
	int turn() const;

	/**
	 * Returns true if this board and the other board are the same.
	 * Used for testing mostly.
	 */
	bool equals(const EnvState& other) const;

	/**
	 * Returns the state for the board obtained by taking the given action
	 * from the current board.
	 * Errors if the given action is illegal.
	 */
	virtual HexState* nextState(int action);
	
	/**
	 * Return true if the given action is legal to take from the current board.
	 */
	bool isLegalAction(int action) const;
	
	/**
	 * Return an action, chosen uniformly at random from the available legal actions.
	 * If there are no legal actions, error.
	 */
	int randomAction() const;
	
	
	/**
	 * Returns the vector that represents the board.
	 */
	vector<int> board() const;


	/* Populates the given vector with the Neural Net representaion of this state. 
	 * Creates channels out of the board.
	 * Each channel is padded on both sides by two columns, and on top and bottom by two rows each.
	 * Channel 1 has a "1" in all spots with Player 1 pieces, and in the top 2 rows and bottom 2 rows.
	 * Channel 2 has a "1" in all spots with Player 2 pieces, and in the left 2 columns and right 2 columns.
	 * The last two elements denote whose turn. [1,0] if Player 1's turn, [0,1] if Player 2's
	 */
	void makeStateVector(vector<double>* state_vector) const;

	/* Returns the number of actions for this game. (dimension x dimension). */
	int numActions() const;

	/* Prints a human-readable representation of the board */
	void printBoard() const;

	/* Returns the board vector, represented as a CSV string. */
	string asCSVString() const; 

	/**
	 * If the given piece is a 1, returns "X", if -1, returns "O", and if 0, returns "_".
	 * Useful for printing the board.
	 */
	string pieceAsString(int piece) const;





private:

	/** 
	 * Returns the piece at the given position (1, -1 or 0).
	 * Errors if the given position is out of range.
	 */
	int at(int pos) const;
	/**
	 * Returns the row index of a given position.
	 * Errors if the given position is out of range.
	 */
	int row(int pos) const;

	/**
	 * Returns the column index of a given position. 
	 * Errors if the given position is out of range.
	 */
	int col(int pos) const;

	/**
	 * Returns the index of the position at the given row and col.
	 */
	int pos(int row, int col) const;

	/**
	 * Returns the index of the northwest neighbor, or -1 if none exists.
	 */
	int northwestNeighbor(int pos) const;

	/**
	 * Returns the index of the northeast neighbor, or -1 if none exists.
	 */
	int northeastNeighbor(int pos) const;

	/**
	 * Returns the index of the southwest neighbor, or -1 if none exists.
	 */
	int southwestNeighbor(int pos) const;

	/**
	 * Returns the index of the southeast neighbor, or -1 if none exists.
	 */
	int southeastNeighbor(int pos) const;

	/**
	 * Returns the index of the west neighbor, or -1 if none exists.
	 */
	int westNeighbor(int pos) const;

	/**
	 * Returns the index of the east neighbor, or -1 if none exists.
	 */
	int eastNeighbor(int pos) const;

	/**
	 * Returns a vector of the neighbors of the given position (all of them that aren't -1).
	 */
	vector<int> neighbors(int pos) const;


	
	/**
	 * Returns true if there is a southward path connecting Player 1's pieces from the given start position
	 * to some spot in the given end row.
	 * The visited bitvector is to assist with the path traversal algorithm.
	 * Errors if the given position or the given end row is out of range.
	 */
	bool southwardPathExists(int start_pos, int end_row, vector<bool>& visited) const;

	/**
	 * Returns true if there is an eastward path connecting Player 2's pieces from the given start position
	 * to some spot in the given end col.
	 * The visited bitvector is to assist with the path traversal algorithm.
	 * Errors if the given position or the given end col is out of range.
	 */
	bool eastwardPathExists(int start_pos, int end_col, vector<bool>& visited) const;

	/**
	 * Returns true if there is a path of Player 1 pieces from the north row to the south row.
	 */
	bool northSouthPathExists() const;

	/**
	 * Returns true if there is a path of Player 2 pieces from the west col to the east col.
	 */
	bool westEastPathExists() const;


	/**
	 * Returns 1 if Player 1 wins this board, -1 if Player 2 wins, and 0 if nobody wins.
	 */
	int determineWinner() const;


	/*** Utility helper functions ***/

	/**
	 * Prints one row of the board in a nice format.
	 */
	void printRow(int row) const;

	int _dimension;
	vector<int> _board;
	int _turn;
	bool _is_terminal;
	double _winner;
	string _reward_type;
	set<int> _legal_actions;
	
};



 

#endif