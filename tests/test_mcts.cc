#include <iostream>


#include "test_mcts.h"
#include "../src/mcts.h"
#include "test_utils.h"
//#include "test_tictactoe.h"

using namespace std;

vector<int> _e = {0,0,0,0,0,0,0,0,0};
vector<int> _p1ws = {1,0,0,1,-1,0,1,0,-1};
vector<int> _p2ws = {1,0,1,-1,-1,-1,0,1,0};
vector<int> _p1wc = {1,-1,1,-1,1,-1,1,-1,1};
vector<int> _p2wc = {-1,0,1,0,-1,1,1,0,-1};
vector<int> _snw = {1,0,0,0,-1,0,0,0,1};
vector<int> _cnw = {1,0,1,-1,-1,1,1,-1,-1};
vector<int> _fm7 = {0,0,0,0,0,0,0,1,0};
vector<int> _bm3 = {1,0,0,-1,-1,0,0,0,1};
vector<int> _wm1 = {1,1,1,-1,-1,1,1,-1,-1};

/*Tictactoe emp;
Tictactoe p1win_simple(p1ws);
Tictactoe p2win_simple(p2ws);
Tictactoe p1win_complex(p1wc);
Tictactoe p2win_complex(p2wc);
Tictactoe simple_nowin(snw);
Tictactoe complex_nowin(cnw);
Tictactoe first_move7(fm7);
Tictactoe black_move3(bm3);
Tictactoe white_move1(wm1);*/

void assertTrueMCTS(bool cond, string test_name, string message) {
	string test_file = "test_mcts.cc";
	assertWithMessage(cond, test_file, test_name, message);
}

void compareVectors(vector<int> exp, vector<int>* actual) {
	int len = exp.size();
	assertTrueMCTS(exp.size() == actual->size(), "compareVectors", "vectors of unequal sizes " + to_string(exp.size()) + " and " + to_string(actual->size()));
	for (int pos = 0; pos < len; pos++) {
		assertTrueMCTS(exp[pos] == actual->at(pos), "compareVectors", "position " + to_string(pos) + " unequal");
	}
}


void testNodeMakeChild() {
	Tictactoe* emp = new Tictactoe(_e);
	MCTS_Node* node = new MCTS_Node(emp);

	assertTrueMCTS(node->getParent() == NULL, "testNodeMakeChild", "initially, parent should be NULL");
	assertTrueMCTS(node->getChild(5) == NULL, "testNodeMakeChild", "initially children should be NULL");

	MCTS_Node* c1 = node->makeChild(5);
	assertTrueMCTS(c1->getParent() == node, "testNodeMakeChild", "parent should be set");
	assertTrueMCTS(node->getChild(5) == c1, "testNodeMakeChild", "child should be set");
	compareVectors({0,0,0,0,0,1,0,0,0}, c1->makeStateVector()->asVector());

	MCTS_Node* c2 = c1->makeChild(1);
	assertTrueMCTS(c2->getParent() == c1, "testNodeMakeChild", "parent should be set");
	assertTrueMCTS(c1->getChild(1) == c2, "testNodeMakeChild", "child should be set");
	compareVectors({0,-1,0,0,0,1,0,0,0}, c2->makeStateVector()->asVector());

	// test making child of terminal action
	Tictactoe* p1win_simple = new Tictactoe(_p1ws);
	MCTS_Node* term_node = new MCTS_Node(p1win_simple);
	MCTS_Node* term_child = term_node->makeChild(3);
	assertTrueMCTS(term_node == term_child, "testNodeMakeChild", "nothing should happen when making child of terminal state");
	assertTrueMCTS(term_node->getParent() == NULL, "testNodeMakeChild", "terminal state parent should be NULL");
	assertTrueMCTS(term_node->getChild(3) == NULL, "testNodeMakeChild", "terminal state children should be NULL");

	cout << "Done with test node make child" << endl;

}

void testNodeConstructor() {
	Tictactoe* simple_nowin = new Tictactoe(_snw);
	MCTS_Node* node = new MCTS_Node(simple_nowin);
	assertTrueMCTS(true, "testNodeConstructor", "Basic assertion");
	assertTrueMCTS(!node->isTerminal(), "testNodeConstructor", "should not be terminal");
	assertTrueMCTS(node->isRoot(), "testNodeConstructor", "should be root");
	assertTrueMCTS(!node->simulationsFinished(), "testNodeConstructor", "simulations are not finished");
	assertTrueMCTS(node->getDepth() == 0, "testNodeConstructor", "initial depth is 0");
	assertTrueMCTS(node->neverSubmittedToNN(), "testNodeConstructor", "initially have not submitted to NN");
	assertTrueMCTS(node->awaitingNNResults(), "testNodeConstructor", "initially awaiting NN Results");

	node->submittedToNN();
	assertTrueMCTS(!node->neverSubmittedToNN(), "testNodeConstructor", "just submitted to NN");
	node->receivedNNResults();
	assertTrueMCTS(!node->awaitingNNResults(), "testNodeConstructor", "just received NN results");

	vector<int> expected_sv = {1,0,0,0,-1,0,0,0,1};
	vector<int>* sv = node->makeStateVector()->asVector();
	for (int pos = 0; pos < 9; pos++) {
		assertTrueMCTS(expected_sv[pos] == sv->at(pos), "testNodeConstructor", "unequal position" + to_string(pos));
	}
	cout << "Done with test node constructor" << endl;

}


void basicSimulation() {
	Tictactoe* emp = new Tictactoe(_e);
	MCTS_Node* node = (new MCTS_Node(emp))->sampleActions(false); 
	int MAX_DEPTH=6;

	vector<int>* init_ad_vec = new vector<int>(9, 0);
	ActionDistribution* init_ad = new ActionDistribution(init_ad_vec);

	vector<int>* test_ad_vec = new vector<int>(9);
	for (int pos = 0; pos < 9; pos++) {
		test_ad_vec->at(pos) = pos / 36;
	}
	ActionDistribution* test_ad = new ActionDistribution(test_ad_vec);
	
	// initial run
	node = runMCTS(node, init_ad, MAX_DEPTH);
	StateVector* sv = node->getStateVector();
	compareVectors({0,0,0,0,0,0,0,0,0}, sv->asVector());
	assertTrueMCTS(!node->neverSubmittedToNN(), "basicSimulation", "node should now have submitted to NN");
	assertTrueMCTS(node->awaitingNNResults(), "basicSimulation", "node should still be awaiting NN Results");

	// second run
	node = runMCTS(node, test_ad, MAX_DEPTH);
	sv = node->getStateVector();
	// should choose action 0, create child 0, submit him to NN queue, return child 0 sv, and set node to child 0
	vector<int> exp_sv = {1,0,0,0,0,0,0,0,0};
	// the child node should have submitted to NN and be awaiting results
	assertTrueMCTS(!node->neverSubmittedToNN(), "basicSimulation", "child node should now no longer be awaiting NN results");
	assertTrueMCTS(node->awaitingNNResults(), "basicSimulation", "child node should be awaiting nn results");
	compareVectors(exp_sv, sv->asVector());
	compareVectors(exp_sv, node->makeStateVector()->asVector());

	// third run
	node = runMCTS(node, test_ad, MAX_DEPTH);
	sv = node->getStateVector();
	// should choose action 1 because child 0 is illegal
	exp_sv = {1,-1,0,0,0,0,0,0,0};
	// the child node should have submitted to NN and be awaiting results
	assertTrueMCTS(!node->neverSubmittedToNN(), "basicSimulation", "child node should now no longer be awaiting NN results");
	assertTrueMCTS(node->awaitingNNResults(), "basicSimulation", "child node should be awaiting nn results");
	compareVectors(exp_sv, sv->asVector());
	compareVectors(exp_sv, node->makeStateVector()->asVector());

	cout << "Done with basic simulation" << endl;

}

void testChooseBestAction() {
	Tictactoe* simple_nowin = new Tictactoe(_snw);
	MCTS_Node* node = (new MCTS_Node(simple_nowin))->sampleActions(false);

	MCTS_Node* c1 = node->chooseBestAction();
	compareVectors({1,-1,0,0,-1,0,0,0,1}, c1->makeStateVector()->asVector());

	MCTS_Node* c2 = c1->chooseBestAction();
	compareVectors({1,-1,1,0,-1,0,0,0,1}, c2->makeStateVector()->asVector());

	MCTS_Node* c3 = c2->chooseBestAction();
	compareVectors({1,-1,1,-1,-1,0,0,0,1}, c3->makeStateVector()->asVector());	

	MCTS_Node* c4 = c3->chooseBestAction();
	compareVectors({1,-1,1,-1,-1,1,0,0,1}, c4->makeStateVector()->asVector());

	MCTS_Node* c5 = c4->chooseBestAction();
	assertTrueMCTS(c5 == c4, "testChooseBestAction", "terminal states have no best action");
	assertTrueMCTS(c4->isTerminal(), "testChooseBestAction", "c4 is terminal");

	cout << "Done with test choose best action" << endl;
}


void testSingleSimulation() {

	int MAX_DEPTH = 5;
	Tictactoe* emp = new Tictactoe(_e);
	MCTS_Node* node = (new MCTS_Node(emp))->sampleActions(false); 


	vector<int>* dummy_ad_vec = new vector<int>(9, 0);
	ActionDistribution* dummy_ad = new ActionDistribution(dummy_ad_vec);

	node = runMCTS(node, dummy_ad, MAX_DEPTH); // root submits to nn
	StateVector* sv = node->getStateVector();
	compareVectors({0,0,0,0,0,0,0,0,0}, sv->asVector());
	assertTrueMCTS(node->parent == NULL, "testSingleSimulation", "parent of root is null");

	node = runMCTS(node, dummy_ad, MAX_DEPTH); // root reweakens, chooses action 0, action 0 submits to nn
	sv = node->getStateVector();
	compareVectors({1,0,0,0,0,0,0,0,0}, sv->asVector());
	

	node = runMCTS(node, dummy_ad, MAX_DEPTH); // child 0 chooses action 1, action 1 submits to nn
	sv = node->getStateVector();
	compareVectors({1,-1,0,0,0,0,0,0,0}, sv->asVector());
	

	node = runMCTS(node, dummy_ad, MAX_DEPTH); // child 1 chooses action 2, action 2 submits to nn
	sv = node->getStateVector();
	compareVectors({1,-1,1,0,0,0,0,0,0}, sv->asVector());
	

	node = runMCTS(node, dummy_ad, MAX_DEPTH); // child 2 chooses action 3, action 3 submits to nn
	sv = node->getStateVector();
	compareVectors({1,-1,1,-1,0,0,0,0,0}, sv->asVector());
	assertTrueMCTS(node->getDepth() == 4, "testSingleSimulation", "Depth should now be 4");

	cout << "following call will enter rollout" << endl;
	node = runMCTS(node, dummy_ad, MAX_DEPTH); // child 3 chooses action 4, action 4 is at max depth, perform rollout, then propagate stats


	// check that N(S) is updated for 
	MCTS_Node* root_node = node;
	int reward = root_node->R(0);
	assertTrueMCTS(root_node->N() == 1 && root_node->N(0) == 1 && root_node->R(0) == reward, "testSingleSimulation", "incorrect stats for root node");

	MCTS_Node* action0 = root_node->getChild(0);
	assertTrueMCTS(action0->N() == 1 && action0->N(1) == 1 && action0->R(1) == reward, "testSingleSimulation", "incorrect stats for action0");

	MCTS_Node* action1 = action0->getChild(1);
	assertTrueMCTS(action1->N() == 1 && action1->N(2) == 1 && action1->R(2) == reward, "testSingleSimulation", "incorrect stats for action1");

	MCTS_Node* action2 = action1->getChild(2);
	assertTrueMCTS(action2->N() == 1 && action2->N(3) == 1 && action2->R(3) == reward, "testSingleSimulation", "incorrect stats for action2");

	MCTS_Node* action3 = action2->getChild(3);
	assertTrueMCTS(action3->N() == 1 && action3->N(4) == 1 && action3->R(4) == reward, "testSingleSimulation", "incorrect stats for action3");


	// check num simulations for root node is incremented
	assertTrueMCTS(root_node->numSimulationsFinished() == 1 && !root_node->simulationsFinished(), "testSingleSimulation", "incorrect numSimulationsFinished");

	cout << "Done with testSingleSimulation" << endl;
}

void printActionDistribution(vector<int>* ad) {
	cout << ad->at(0) << "\t" << ad->at(1) << "\t" << ad->at(2) << endl;
	cout << ad->at(3) << "\t" << ad->at(4) << "\t" << ad->at(5) << endl;
	cout << ad->at(6) << "\t" << ad->at(7) << "\t" << ad->at(8) << endl;
}

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

void testFinish() {


	int MAX_DEPTH = 5;
	

	vector<int>* dummy_ad_vec = new vector<int>(9, 0);
	ActionDistribution* dummy_ad = new ActionDistribution(dummy_ad_vec);

	Tictactoe* board;
	MCTS_Node* node;
	vector<int>* root_ad;

	// test that player 1 knows how to win
	board = new Tictactoe({1,0,-1,-1,1,0,0,0,0});
	node = (new MCTS_Node(board))->sampleActions(true);
	while (!node->simulationsFinished()) {
		node = runMCTS(node, dummy_ad, MAX_DEPTH);
	}

	assertTrueMCTS(node->simulationsFinished(), "testFinish", "incorrect numSimulationsFinished");

	cout << endl << "Root action Distribution (should pick the win):" << endl;
	node->state->printBoard(); cout << endl;
	root_ad = node->getActionCounts();
	printActionDistribution(root_ad);
	assertTrueMCTS(indexOfMax(root_ad) == 8, "testFinish", "Player 1 Should take the win");

	// test that player 2 knows how to win
	board = new Tictactoe({1,0,1,-1,0,-1,0,1,0});
	node = (new MCTS_Node(board))->sampleActions(true);
	while (!node->simulationsFinished()) {
		node = runMCTS(node, dummy_ad, MAX_DEPTH);
	}

	assertTrueMCTS(node->simulationsFinished(), "testFinish", "incorrect numSimulationsFinished");

	cout << endl << "Root action Distribution (should pick the win):" << endl;
	node->state->printBoard(); cout << endl;
	root_ad = node->getActionCounts();
	printActionDistribution(root_ad);
	assertTrueMCTS(indexOfMax(root_ad) == 4, "testFinish", "Player 2 Should take the win");

	


}


void testBlock() {

	int MAX_DEPTH = 5;
	

	vector<int>* dummy_ad_vec = new vector<int>(9, 0);
	ActionDistribution* dummy_ad = new ActionDistribution(dummy_ad_vec);

	Tictactoe* board;
	MCTS_Node* node;
	vector<int>* root_ad;



	// test that player 1 knows how to block
	board = new Tictactoe({1,0,-1,0,-1,0,0,0,1});
	node = (new MCTS_Node(board, true, 1000))->sampleActions(true);
	while (!node->simulationsFinished()) {
		node = runMCTS(node, dummy_ad, MAX_DEPTH);
	}

	assertTrueMCTS(node->simulationsFinished(), "testBlock", "incorrect numSimulationsFinished");

	cout << endl << "Root action Distribution (should block):" << endl;
	node->state->printBoard(); cout << endl;
	root_ad = node->getActionCounts();
	printActionDistribution(root_ad);
	assertTrueMCTS(indexOfMax(root_ad) == 6, "testBlock", "Player 1 Should block");




	// test that player 2 knows how to block
	board = new Tictactoe({1,0,0,1,-1,1,0,0,-1});
	node = (new MCTS_Node(board, true, 1000))->sampleActions(true);
	while (!node->simulationsFinished()) {
		node = runMCTS(node, dummy_ad, MAX_DEPTH);
	}

	assertTrueMCTS(node->simulationsFinished(), "testBlock", "incorrect numSimulationsFinished");

	cout << endl << "Root action Distribution (should block):" << endl;
	node->state->printBoard(); cout << endl;
	root_ad = node->getActionCounts();
	printActionDistribution(root_ad);
	assertTrueMCTS(indexOfMax(root_ad) == 6, "testBlock", "Player 2 Should block");
}



void runMctsTests() {
	cout << "Running MCTS Tests..." << endl;
	testNodeConstructor();
	testNodeMakeChild();
	basicSimulation();
	testChooseBestAction();
	//testSingleSimulation(); // breaks if we remove t < 1 condition in runMCTS loop
	testFinish();
	testBlock();

	cout << "Done running MCTS Tests." << endl << endl;
}