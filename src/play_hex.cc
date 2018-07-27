#include "play_hex.h"

#include <iostream>

using namespace std;













int indexOfMax(vector<int>* vec) {
	int max_index = 0;
	int max_val = INT_MIN;
	for (int i = 0; i < vec->size(); i++) {
		if (vec->at(i) > max_val) {
			max_index = i;
			max_val = vec->at(i);
		}
	}
	return max_index;
}


void declareWinner(int winner, double reward) {
	if (winner == 1) {
		cout << "Player X Wins, with a reward of " << reward << endl;
	} 
	if (winner == -1) {
		cout << "Player O Wins, with a reward of " << reward << endl;
	}
	if (winner == 0) {
		cout << "Draw" << endl;
	}

	cout << endl << endl;
}

int vanillaMCTSAgent(Tictactoe* board, ActionDistribution* ad, int max_depth, int num_simulations=1000, bool sample_actions=true) {
	MCTS_Node* node = (new MCTS_Node(board, true, num_simulations))->sampleActions(sample_actions);
	node = runAllSimulations(node, ad, max_depth);
	vector<int>* action_counts = node->getActionCounts();
	int action = indexOfMax(action_counts);
	return action;
}

int userAgent(Tictactoe* board) {
	// read input
	int action;
	bool legal_action = false;
	while (!legal_action) {
		cout << "Enter an action number: ";
		cin >> action;
		cout << endl;
		legal_action = board->isLegalAction(action);
		if (!legal_action) {
			cout << "Action " << action << " is illegal" << endl;
		}
	}
	return action;
}


void vanillaMCTSOnePlayer(int user_player=1) {
	if (user_player == 1) {
		UserTictactoeAgent p1_agent;
		VanillaMCTSTictactoeAgent p2_agent;
		playTictactoe(p1_agent, p2_agent);
	}

	else if (user_player == -1) {
		VanillaMCTSTictactoeAgent p1_agent;
		UserTictactoeAgent p2_agent;
		playTictactoe(p1_agent, p2_agent);
	}
	
	else {
		cout << "Error, there is no player " << user_player << endl;
	}
}

void playTictactoe(TictactoeAgent& p1_agent, TictactoeAgent& p2_agent) {

	Tictactoe* board = new Tictactoe({0,0,0,0,0,0,0,0,0});;

	int action;
	while (!board->isTerminalState()) {
		cout << endl;
		board->printBoard();
		cout << endl;

		if (board->turn() == 1) {
			action = p1_agent.getAction(board);
		}

		if (board->turn() == -1) {
			action = p2_agent.getAction(board);
		}

		board = board->nextState(action);
		
	}

	double reward = ((double) board->reward());
	int winner = board->winner();
	declareWinner(winner, reward);
}


int TictactoeAgent::getAction(Tictactoe* board) {
	return -1;
}

TictactoeAgent::TictactoeAgent() {

}

UserTictactoeAgent::UserTictactoeAgent() {

}

VanillaMCTSTictactoeAgent::VanillaMCTSTictactoeAgent() {

}

int UserTictactoeAgent::getAction(Tictactoe* board) {
	// read input
	int action;
	bool legal_action = false;
	while (!legal_action) {
		cout << "Enter an action number: ";
		cin >> action;
		cout << endl;
		legal_action = board->isLegalAction(action);
		if (!legal_action) {
			cout << "Action " << action << " is illegal" << endl;
		}
	}
	return action;
}

int VanillaMCTSTictactoeAgent::getAction(Tictactoe* board) {
	vector<double>* dummy_ad_vec = new vector<double>(9, 0.0);
	ActionDistribution* dummy_ad = new ActionDistribution(dummy_ad_vec);

	MCTS_Node* node = (new MCTS_Node(board, true, num_simulations))->sampleActions(sample_actions);
	node = runAllSimulations(node, dummy_ad, max_depth);
	vector<int>* action_counts = node->getActionCounts();
	int action = indexOfMax(action_counts);
	return action;
}

void VanillaMCTSTictactoeAgent::setNumSimulations(int num_simulations) {
	this->num_simulations = num_simulations;
}

void VanillaMCTSTictactoeAgent::sampleActions(bool sample_actions) {
	this->sample_actions = sample_actions;
}

void VanillaMCTSTictactoeAgent::setMaxDepth(int max_depth) {
	this->max_depth = max_depth;
}

int VanillaMCTSTictactoeAgent::numSimulations() {
	return this->num_simulations;
}

int VanillaMCTSTictactoeAgent::maxDepth() {
	return this->max_depth;
}

bool VanillaMCTSTictactoeAgent::doesSampleActions() {
	return this->sample_actions;
}



int main() {
	int user_player = 1;
	vanillaMCTSOnePlayer(user_player);


}