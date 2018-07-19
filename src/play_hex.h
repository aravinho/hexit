#ifndef PLAY_HEX_H

#include <vector>
#include "mcts.h"

using namespace std;

class TictactoeAgent {
public:
	TictactoeAgent();
	virtual int getAction(Tictactoe* board);
};

class VanillaMCTSTictactoeAgent : public TictactoeAgent {
public:
	VanillaMCTSTictactoeAgent();
	void setNumSimulations(int num_simulations);
	void sampleActions(bool sample_actions);
	void setMaxDepth(int max_depth);

	int numSimulations();
	bool doesSampleActions();
	int maxDepth();

	int getAction(Tictactoe* board);
private:
	int num_simulations = 1000;
	int max_depth = 5;
	bool sample_actions = true;
};

class UserTictactoeAgent : public TictactoeAgent {
public:
	UserTictactoeAgent();
	int getAction(Tictactoe* board);

};

void playTictactoe(TictactoeAgent& p1_agent, TictactoeAgent& p2_agent);

#endif

/*
MCTS_Node* node = (new MCTS_Node(board, true, num_simulations))->sampleActions(sample_actions);
	node = runAllSimulations(node, ad, max_depth);
	vector<int>* action_counts = node->getActionCounts();
	int action = indexOfMax(action_counts);
	return action;*/