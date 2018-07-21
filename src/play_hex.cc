#include "play_hex.h"

#include <iostream>

using namespace std;


/*TicTacToe::TicTacToe(const wxString& title)
       : wxFrame(NULL, -1, title, wxPoint(-1, -1), wxSize(580, 640))
{
  SetBackgroundColour(wxColour(240,240,240));
   
  menubar = new wxMenuBar;
  menuGame = new wxMenu;
  
  menuGame->Append(new wxMenuItem(menuGame, ID_MNG, wxT("&New Game"), wxT("")));
  menuGame->Append(new wxMenuItem(menuGame, ID_MAB, wxT("&About"), wxT("")));
  menuGame->AppendSeparator();
  menuGame->Append(new wxMenuItem(menuGame, ID_MEX, wxT("E&xit"), wxT("")));
  
  menubar->Append(menuGame, wxT("&Game"));
  
  SetMenuBar(menubar);
  
  who = 'X';
  moves = 0;
  ComputerOpponent = false;
  hasAnyWon = false;
  
  scoreX = scoreO = scoreTies = 0;

  sizer = new wxBoxSizer(wxVERTICAL);
  
  player = new wxTextCtrl(this, ID_TP, wxT(" Current: Player 1 (X) "), wxPoint(-1, -1), wxSize(-1, -1), wxTE_CENTER | wxTE_READONLY);
  score = new wxTextCtrl(this, ID_TS, wxT("Score: Player X = 0  ||  Player O = 0  ||  Ties = 0"), wxPoint(-1, -1), wxSize(-1, -1), wxTE_CENTER | wxTE_READONLY);
  cCo = new wxCheckBox(this, ID_CCO, wxT("Against the Computer?"), wxPoint(-1, -1));
  
  bNew = new wxButton(this, ID_BNG, wxT("New Game"));
  
  b00 = new wxButton(this, ID_B00, wxT(""));
  b01 = new wxButton(this, ID_B01, wxT(""));
  b02 = new wxButton(this, ID_B02, wxT(""));
  b10 = new wxButton(this, ID_B10, wxT(""));
  b11 = new wxButton(this, ID_B11, wxT(""));
  b12 = new wxButton(this, ID_B12, wxT(""));
  b20 = new wxButton(this, ID_B20, wxT(""));
  b21 = new wxButton(this, ID_B21, wxT(""));
  b22 = new wxButton(this, ID_B22, wxT(""));
  
  wxFont df = wxFont(40, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
  wxFont dbf = wxFont(13, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
  wxFont scoreFont = wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
  
  wxStaticText *titleTextPart1 = new  wxStaticText(this, -1, wxT("A Game of"));
  titleTextPart1->SetFont(wxFont(14, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  titleTextPart1->SetForegroundColour(wxColour(13, 132, 123));
  
  wxStaticText *titleText = new  wxStaticText(this, -1, wxT("Tic Tac Toe"));
  titleText->SetFont(wxFont(30, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  titleText->SetForegroundColour(wxColour(13, 132, 200));
  
  wxStaticText *authorText = new  wxStaticText(this, -1, wxT("by Afaan Bilal"));
  authorText->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  authorText->SetForegroundColour(wxColour(13, 132, 123));
    
  score->SetFont(scoreFont);
  score->SetForegroundColour(wxColour(80,80,80));  
  player->SetForegroundColour(wxColour(80,180,100));
  player->SetFont(dbf);
  bNew->SetFont(dbf);
  cCo->SetFont(dbf);
  
  b00->SetFont(df);
  b01->SetFont(df);
  b02->SetFont(df);
  b10->SetFont(df);
  b11->SetFont(df);
  b12->SetFont(df);
  b20->SetFont(df);
  b21->SetFont(df);
  b22->SetFont(df);
  
  sizer->Add(titleTextPart1, 0, wxALIGN_LEFT | wxALL, 10);
  sizer->Add(titleText, 0, wxALIGN_CENTER | wxALL, 10);
  sizer->Add(authorText, 0, wxALIGN_RIGHT | wxALL, 10);
  sizer->Add(score, 0, wxEXPAND | wxALL, 10);
  sizer->Add(player, 0, wxEXPAND | wxALL, 10);
  sizer->Add(cCo, 0, wxALIGN_CENTER | wxALL, 10);
  
  gs = new wxGridSizer(4, 3, 3, 3);
    
  gs->Add(new wxStaticText(this, -1, wxT("")), 0, wxALIGN_CENTER, 0);
  gs->Add(bNew, 0, wxEXPAND);
  gs->Add(new wxStaticText(this, -1, wxT("")), 0, wxALIGN_CENTER, 0);
  
  gs->Add(b00, 0, wxEXPAND); 
  gs->Add(b01, 0, wxEXPAND);
  gs->Add(b02, 0, wxEXPAND);
  gs->Add(b10, 0, wxEXPAND);
  gs->Add(b11, 0, wxEXPAND);
  gs->Add(b12, 0, wxEXPAND);
  gs->Add(b20, 0, wxEXPAND);
  gs->Add(b21, 0, wxEXPAND);
  gs->Add(b22, 0, wxEXPAND);

  sizer->Add(gs, 1, wxEXPAND);
  SetSizer(sizer);
  SetMinSize(wxSize(580, 640));
  
  Connect(ID_BNG, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnNewGame));
  Connect(ID_MNG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TicTacToe::OnNewGame));
  Connect(ID_MAB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TicTacToe::OnAbout));
  Connect(ID_MEX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TicTacToe::OnExit));
  Connect(ID_CCO, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(TicTacToe::OnCCO));
  
  Connect(ID_B00, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B01, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B02, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B10, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B11, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B12, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B20, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B21, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  Connect(ID_B22, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TicTacToe::OnClick));
  
  Centre();
  Reset();
  DarkenButtons(true);
  EnableButtons(false);
}*/












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
	vector<int>* dummy_ad_vec = new vector<int>(9, 0);
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