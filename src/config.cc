#include "config.h"

using namespace std;

/* Default values used by all parts of the system. */

set<string> GAME_OPTIONS = {"tictactoe"};

string DEFAULT_GAME = "hex";
int DEFAULT_HEX_DIM = 5;
string DEFAULT_HEX_REWARD_TYPE = "win_fast";

int DEFAULT_NUM_STATES = 4096;

int DEFAULT_NUM_SIMULATIONS = 1000;
int DEFAULT_MAX_DEPTH = 4;

int DEFAULT_MINIBATCH_SIZE = 256;
int DEFAULT_NUM_THREADS = 4;
int DEFAULT_LOG_EVERY = 512;

int DEFAULT_STATES_PER_FILE = pow(2, 20);

bool DEFAULT_SAMPLE_ACTIONS = true;
bool DEFAULT_REQUIRES_NN = false;
bool DEFAULT_USE_RAVE = false;

double DEFAULT_C_B = 0.05;
double DEFAULT_C_RAVE = 3000;
double DEFAULT_W_A = 40;

int DEFAULT_START_AT = 0;



