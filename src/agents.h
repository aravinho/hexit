#ifndef AGENTS_H
#define AGENTS_H

#include <vector>
#include "mcts.h"
#include "inference.h"


using namespace std;


/**
 * Parent class for all types of Agents.
 * All inheriting agent classes must implement getAction(), which, given a state, returns an action this agent recommends.
 */
class GameAgent {

public:

	/* Predicts an action to take from the given state. */	
	virtual int getAction(EnvState* state) const = 0;

	/**
	 * Makes action predictions for each of the BATCH_SIZE states in active_states. Skips if that episode is already finished (FINISHED_EPISODES is a bitvector).
	 * Advances each state to the state resulting from taking the predicted action.
	 * If any of the episodes are now finished, mark them as such in FINISHED_EPISODES.
	 * Update player1_wins or player2_wins accordingly.
	 * Returns the number of finished episodes.
	 */
	virtual int makeBatchActions(int batch_size, vector<EnvState*>* active_states, vector<bool>* finished_episodes,
		int* player1_wins, int* player2_wins, Session* session=NULL) const;

	/* Default is false. */
	virtual bool isNNAgent() const;

	/* Errors since default is not an NN Agent. */
	virtual Session* createSession() const;

};


/** 
 * Uses a user's actions.
 */
class UserAgent: public GameAgent {

public:

	UserAgent();

	/* Prompts the user for an action to take from the given state. */
	int getAction(EnvState* state) const;

};


/**
 * Uses inference on a previously trained neural net to decide an action.
 */
class NNAgent: public GameAgent {

public:

	/**
	 * Initializes this agent.
	 * The model for this agent is stored at the given MODEL_PATH.
	 */
	NNAgent(const string& model_path, const ArgMap& arg_map);

	/* Predicts an action to take from the given state. */
	int getAction(EnvState* state) const;

	/** 
	 * Starts up a Tensorflow session.
	 * Reads in the metagraph stored at this->model_path, and adds it to the session.
	 * Restores all the ops from the TF collection.
	 * All the heavy lifting is done in the restoreModel function in inference.cc.
	 * Returns a pointer to the created session.
	 */
	virtual Session* createSession() const override;
	
	/**
	 * Makes action predictions for each of the BATCH_SIZE states in active_states. Skips if that episode is already finished (FINISHED_EPISODES is a bitvector).
	 * Advances each state to the state resulting from taking the predicted action.
	 * If any of the episodes are now finished, mark them as such in FINISHED_EPISODES.
	 * Update player1_wins or player2_wins accordingly.
	 * Returns the number of finished episodes.
	 */
	virtual int makeBatchActions(int batch_size, vector<EnvState*>* active_states, vector<bool>* finished_episodes,
		int* player1_wins, int* player2_wins, Session* session=NULL) const override;

	/** True. */
	virtual bool isNNAgent() const override;

private:

	string model_path;

};


/** Chooses legal actions randomly. */
class RandomAgent: public GameAgent {

public:
	RandomAgent();
	
	/* Predicts an action to take from the given state. */
	int getAction(EnvState* state) const;
};


/**
 * Chooses actions from a state by running several MCTS simulations, each from this state,
 * and then choosing the most visited action from the root node.
 */
class MCTSAgent: public GameAgent {

public:

	/* ARG_MAP contains options on how to configure MCTS, such as num_simulations, hyperparameter values, reward type, etc. */
	MCTSAgent(const ArgMap& arg_map);
	
	/* Predicts an action to take from the given state. */
	int getAction(EnvState* state) const;

private:

	int num_simulations;
	int max_depth;
	int use_rave;
	bool sample_actions;
	double c_b;
	double c_rave;
};



#endif
