#ifndef ENV_STATE_H
#define ENV_STATE_H

#include "utils.h"

#include <vector>

using namespace std;

/* Represents a particular state in an MDP.
 * This is an abstract class, and any particular MDP must provide a class that inherits from this class.
 */
class EnvState {

public:

	/**
	 * Returns which player has won in this state (0 if non-terminal state).
	 */
	virtual int winner() const = 0;

	/**
	 * Returns true if the current board is a terminal state,
	 * and false otherwise.
	 */
	virtual bool isTerminalState() const = 0;

	/**
	 * Returns 0 if the current board is in a nonterminal state,
	 * Positive if X has won, and negative if O has won.
	 */
	virtual double reward() const = 0;

	/**
	 * Returns the state for the board obtained by taking the given action
	 * from the current board.
	 * Errors if the given action is illegal.
	 */
	virtual EnvState* nextState(int action) = 0;
	
	/**
	 * Return true if the given action is legal to take from the current board.
	 */
	virtual bool isLegalAction(int action) const = 0;
	
	/**
	 * Return an action, chosen uniformly at random from the available legal actions.
	 * If there are no legal actions, error.
	 */
	virtual int randomAction() const = 0;
	
	/**
	 * Return 1 if it is Player 1's turn in the current board, or -1 if Player 2's turn.
	 * Error if the board is invalid or in a terminal state.
	 */
	virtual int turn() const = 0;

	/**
	 * Returns the vector that represents the board.
	 */
	virtual vector<int> board() const = 0;


	/* Populates the given vector with the Neural Net representaion of this state. */
	virtual void makeStateVector(vector<double>* state_vector) const = 0;

	/* Returns the number of actions for this game. (dimension x dimension). */
	virtual int numActions() const = 0;

	/* Prints a human-readable representation of the board */
	virtual void printBoard() const = 0;

	/* Returns the board vector, represented as a CSV string. */
	virtual string asCSVString() const = 0; 
};



EnvState* stateFromCSVString(string game, string csv_string, const ArgMap& options);

#endif