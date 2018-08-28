#include "agents.h"
#include "inference.h"


#include <iostream>

using namespace std;





/**** Default GameAgent ****/

bool GameAgent::isNNAgent() const {
	return false;
}

Session* GameAgent::createSession() const {
	ASSERT(false, "only NNAgents can create a session");
}


bool NNAgent::isNNAgent() const {
	return true;
}


// returns number of episodes finished in this round
int GameAgent::makeBatchActions(int batch_size, vector<EnvState*>* active_states, vector<bool>* finished_episodes,
	int* player1_wins, int* player2_wins, Session* session) const {

	ASSERT(active_states != NULL, "Active states must not be null");
	ASSERT(finished_episodes != NULL, "Finished episodes must not be null");
	ASSERT(active_states->size() >= batch_size, "Must have at least " << batch_size << " states in the active_states vector");
	ASSERT(finished_episodes->size() >= batch_size, "Must have at least " << batch_size << " states in the active_states vector");

	int num_episodes_finished = 0;

	for (int episode_num = 0; episode_num < batch_size; episode_num++) {

		// check that episode is still active. if not, move on
		if (finished_episodes->at(episode_num)) {
			continue;
		}

		// if still active, must make a move
		EnvState* state = active_states->at(episode_num);
		int action = this->getAction(state);
		EnvState* next_state = state->nextState(action);
		active_states->at(episode_num) = next_state;
		delete state;

		// if this move finishes the episode, mark it finished and mark the winner
		if (next_state->isTerminalState()) {
			finished_episodes->at(episode_num) = true;
			num_episodes_finished += 1;
			if (next_state->winner() == 1) {
				*player1_wins += 1;
			}
			if (next_state->winner() == -1) {
				*player2_wins += 1;
			}
		}

	}

	return num_episodes_finished;

} 




/***** NNAgent *****/ 

NNAgent::NNAgent(const string& model_path, const ArgMap& arg_map) {
	this->model_path = model_path;
}

int NNAgent::getAction(EnvState* state) const {
	return 0;
}

Session* NNAgent::createSession() const {

	const string meta_graph_path = this->model_path + ".meta";
    const string model_checkpoint_path = this->model_path;

	// initialize the TF session
    Session* session = NewSession(SessionOptions());
    ASSERT(session != NULL, "Session is NULL");
    // Read in the protobuf graph we exported, and add it to the session
    MetaGraphDef graph_def;
    Status status = restoreModelGraph(session, &graph_def, meta_graph_path, model_checkpoint_path);
    ASSERT(status.ok(), "Error restoring model graph");
    logTime("Successfully loaded metagraph and all nodes from model checkpoint at " + model_checkpoint_path);

    return session;
}

int NNAgent::makeBatchActions(int batch_size, vector<EnvState*>* active_states, vector<bool>* finished_episodes,
	int* player1_wins, int* player2_wins, Session* session) const {

	ASSERT(active_states != NULL, "Active states must not be null");
	ASSERT(finished_episodes != NULL, "Finished episodes must not be null");
	ASSERT(active_states->size() >= batch_size, "Must have at least " << batch_size << " states in the active_states vector");
	ASSERT(finished_episodes->size() >= batch_size, "Must have at least " << batch_size << " states in the active_states vector");
	ASSERT(session != NULL, "Cannot have a null session in NNAgent::makeBatchActions");

	// create a TF feed dict from this batch, and run inference
	vector<pair<string, Tensor>> feed_dict;
	vector<string> output_ops;
	createTensorsFromStates(batch_size, *active_states, &feed_dict, &output_ops);
	vector<Tensor> output_tensors;
	
	Status status = predictBatch(session, feed_dict, output_ops, &output_tensors);

	// unpack the actions for each of the states, by choosing the argmax of the NN's prediction
	vector<int> chosen_actions(batch_size);
	for (int episode_num = 0; episode_num < batch_size; episode_num++) {
		if (finished_episodes->at(episode_num)) {
			continue;
		}
		Tensor action_dist_tensor = output_tensors[0];
		auto action_dist = action_dist_tensor.matrix<float>();
		vector<double> weights(25);
		for (int i = 0; i < 25; i++) {
			weights[i] = action_dist(episode_num, i);
		}
		// argmax or sample?
		int action = argmax(weights);
		if (!active_states->at(episode_num)->isLegalAction(action)) {
			action = active_states->at(episode_num)->randomAction();
		}
		chosen_actions[episode_num] = action;

	}


	int num_episodes_finished = 0;
	// advance all the active episodes by taking the action.
	for (int episode_num = 0; episode_num < batch_size; episode_num++) {
		// check that episode is still active. if not, move on
		if (finished_episodes->at(episode_num)) {
			continue;
		}

		EnvState* state = active_states->at(episode_num);
		int action = chosen_actions[episode_num];
		EnvState* next_state = state->nextState(action);
		active_states->at(episode_num) = next_state;
		delete state;

		// if this episode is now finished, mark it so
		if (next_state->isTerminalState()) {
			finished_episodes->at(episode_num) = true;
			num_episodes_finished += 1;
			if (next_state->winner() == 1) {
				*player1_wins += 1;
			}
			if (next_state->winner() == -1) {
				*player2_wins += 1;
			}
		}
		

	}

	return num_episodes_finished;

}



/**** UserAgent *****/
UserAgent::UserAgent() {

}

int UserAgent::getAction(EnvState* state) const {
	while (true) {
		cout << "Please enter a row (0-indexed): " << endl;
		int row;
		cin >> row;
		cout << "Please enter a column (0-indexed): " << endl;
		int col;
		cin >> col;
		int dim = (int) sqrt(state->numActions());
		int action = (row * dim) + col;
		if (state->isLegalAction(action)) {
			return action;
		}
		cout << "Action " << action << " is illegal. Please try again." << endl;
	}

	// to suppress warnings
	return 0;
}


/**** RandomAgent *****/

RandomAgent::RandomAgent() {

}

int RandomAgent::getAction(EnvState* state) const {
	return state->randomAction();
}





/***** MCTSAgent ****/

MCTSAgent::MCTSAgent(const ArgMap& arg_map) {

	this->num_simulations = arg_map.getInt("num_simulations", DEFAULT_NUM_SIMULATIONS);
	cout << "num_simulations " << this->num_simulations << endl;
	this->max_depth = arg_map.getInt("max_depth", DEFAULT_MAX_DEPTH);
	cout << "max_depth " << this->max_depth << endl;
	this->use_rave = arg_map.getBool("use_rave", DEFAULT_USE_RAVE);
	this->sample_actions = arg_map.getBool("sample_actions", DEFAULT_SAMPLE_ACTIONS);
	this->c_b = arg_map.getDouble("c_b", DEFAULT_C_B);
	this->c_rave = arg_map.getDouble("c_rave", DEFAULT_C_RAVE);

}

int MCTSAgent::getAction(EnvState* state) const {

	MCTS_Node* node = new MCTS_Node(state, true /* is_root */, this->num_simulations, this->sample_actions, false /* requires_nn */, this->use_rave,
		this->c_b, this->c_rave);

	node = runAllSimulations(node, this->max_depth);

	// argmax over mean reward
	vector<double>* mean_rewards = new vector<double>(node->getState()->numActions(), 0.0);
	node->getMeanRewards(mean_rewards);

	// print some debug info
	vector<int>* action_counts = node->getActionCounts();
	int dim = (int) sqrt(node->getState()->numActions());
	//printVector(*action_counts, "Master Action counts:", dim);
	//node->printMeanReward();
	//node->printExplorationTerm();
	
	// choose best action
	int action;
	if (state->turn() == 1) {
		action = argmax(*mean_rewards);
	} else {
		action = argmin(*mean_rewards);
	}
	//int action = argmax(action_counts);

	if (!state->isLegalAction(action)) {
		action = state->randomAction();
	}
	
	return action;

}






// runs in batch mode
pair<int, int> runNNEpisodes(int num_episodes, const GameAgent& p1_agent, const GameAgent& p2_agent, const ArgMap& arg_map) {

	// unpack some arguments
	int batch_size = arg_map.getInt("batch_size", DEFAULT_NN_BATCH_SIZE);
	int log_every = arg_map.getInt("log_every", DEFAULT_LOG_EVERY);
	ASSERT(batch_size > 0 && log_every > 0, "Batch size and log every must be positive");
	string game = arg_map.getString("game", DEFAULT_GAME);
	ASSERT(GAME_OPTIONS.count(game) > 0, "Game " << game << " not supported");
	int display_state = arg_map.getBool("display_state", DEFAULT_DISPLAY_STATE);


    // initialize the TF session(s)
    Session* p1_session = NULL; Session* p2_session = NULL;
    if (p1_agent.isNNAgent()) {
    	p1_session = p1_agent.createSession();
    }
    if (p2_agent.isNNAgent()) {
    	p2_session = p2_agent.createSession();
    }

   
    // determine the number of batches
    int num_batches = num_episodes / batch_size;
    if (num_batches == 0 && num_episodes % num_batches > 0) {
    	num_batches += 1;
    }

    // initialize some statistics that pertain to the entire set of episodes
    int player1_wins = 0; int player2_wins = 0;

    // iterate over all the batches
    for (int batch_num = 0; batch_num < num_batches; batch_num++) {

    	// initialize some stats for this batch of episodes
    	int num_unfinished_episodes = batch_size;
    	vector<bool> finished_episodes(batch_size, false);
    	int turn = 1;

    	// initialize empty states for all the episodes in this batch
    	vector<EnvState*> active_states(batch_size);
    	for (int state_num = 0; state_num < batch_size; state_num++) {
    		active_states[state_num] = initialState(game, arg_map);
    	}

    	
    	int num_moves = 0;
    	// in each iteration of this loop, advance the state of each episode that is still active (has not yet terminated)
    	// once all episodes in this batch are terminated, we are done
    	while (num_unfinished_episodes > 0) {


    		
    		// log if necessary
    		if (num_moves % log_every == 0) {
    			logTime(to_string(num_moves) + " moves have been made in batch " + to_string(batch_num));
    		}

    		// print board if necessary
    		if (display_state) {
    			active_states[0]->printBoard();
    		}

    		// for now to debug profiler
/*    		if (num_moves == 2) {
    			return make_pair(0, 0);
    		}*/


    		int num_episodes_finished;
    		// if it is Player 1's turn, user P1_AGENT.
    		if (turn == 1) {
    			num_episodes_finished = p1_agent.makeBatchActions(batch_size, &active_states, &finished_episodes, &player1_wins, &player2_wins, p1_session);
    		}

    		if (turn == -1) {
    			num_episodes_finished = p2_agent.makeBatchActions(batch_size, &active_states, &finished_episodes, &player1_wins, &player2_wins, p2_session);
    		}

    		// prepare for the next round
    		num_unfinished_episodes -= num_episodes_finished;
    		turn *= -1;
    		num_moves += 1;



    	}

    }
    
    return make_pair(player1_wins, player2_wins);

}



 /* ../bazel-bin/hexit/src/agents \
    --game <game_name> [defaults to "hex"] \
    --num_episodes <number of games> [defaults to 1] \
    --batch_size <batch size for inference mode> [defaults to 128] \
    --hex_dim <dimension of hex game> [unnecessary for non-hex games, defaults to 5] \
    \
    --p1_agent <nn OR user OR random> [defaults to user] \
    --p1_model_path <path to NN model to be used for Agent 1> [unnecessary for non-NN Player 1 Agents] \
    \
    --p2_agent <nn OR user OR random> [defaults to user] \
    --p2_model_dir <path to NN model to be used for Agent 2> [unnecessary for non-NN Player 2 Agents] \
    \
    --log_every <logging frequency F> [optional, prints a message after F episodes are run; defaults to 10] \

    Example:

    ../bazel-bin/hexit/src/agents \
    --game hex \
    --num_episodes 32 \
    --batch_size 32 \
    --hex_dim 5 \
    \
    --p1_agent nn \
    --p1_model_path models/hex/5/test_models/exp_model \
    \
    --p2_agent mcts \
    \
    --log_every 512 \
*/


int main(int argc, char* argv[]) {

	srand(time(NULL));

	// parse all command line arguments into an ArgMap instance
	ArgMap arg_map;
	parseArgs(argc, argv, &arg_map);

	int num_episodes = arg_map.getInt("num_episodes");

	GameAgent* p1_agent;
	GameAgent* p2_agent;

	string p1_agent_type = arg_map.getString("p1_agent", "random");
	if (p1_agent_type == "nn") {
		string p1_model_path = arg_map.getString("p1_model_path");
		p1_agent = new NNAgent(p1_model_path, arg_map);
	} else if (p1_agent_type == "mcts") {
		p1_agent = new MCTSAgent(arg_map);
	} else if (p1_agent_type == "random") {
		p1_agent = new RandomAgent();
	} else if (p1_agent_type == "user") {
		p1_agent = new UserAgent();
	} else {
		ASSERT(false, "Cannot support Player 1 Agent of type " << p1_agent_type);
	}


	string p2_agent_type = arg_map.getString("p2_agent", "random");
	if (p2_agent_type == "nn") {
		string p2_model_path = arg_map.getString("p2_model_path");
		p2_agent = new NNAgent(p2_model_path, arg_map);
	} else if (p2_agent_type == "mcts") {
		p2_agent = new MCTSAgent(arg_map);
	} else if (p2_agent_type == "random") {
		p2_agent = new RandomAgent();
	} else if (p2_agent_type == "user") {
		p2_agent = new UserAgent();
	} else {
		ASSERT(false, "Cannot support Player 2 Agent of type " << p2_agent_type);
	}


	cout << "is p1 an NN? " << p1_agent->isNNAgent() << endl;
	cout << "is p2 an NN? " << p2_agent->isNNAgent() << endl;

	pair<int, int> results = runNNEpisodes(num_episodes, *p1_agent, *p2_agent, arg_map);
	int p1_wins = results.first, p2_wins = results.second;
	cout << "Player 1 (" << p1_agent_type << ") wins: " << p1_wins << endl;
	cout << "Player 2 (" << p2_agent_type << ") wins: " << p2_wins << endl;

	string profiler_log_file = arg_map.getString("profiler_log_file", DEFAULT_PROFILER_LOG_PATH);
	profiler.log(profiler_log_file);

	return 0;




}