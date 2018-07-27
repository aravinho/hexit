#ifndef ENV_STATE_H
#define ENV_STATE_H

/* Represents a particular state in an MDP.
 * This is an abstract class, and any particular MDP must provide a class that inherits from this class.
 */
class EnvState {

public:

	/* Returns which player has won in this state (0 if non-terminal state). */
	virtual int winner() = 0;
};

#endif