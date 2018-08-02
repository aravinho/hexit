#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <set>
#include <math.h>

using namespace std;


/* Default config options */

extern set<string> GAME_OPTIONS;

extern string DEFAULT_GAME;
extern int DEFAULT_HEX_DIM;
extern string DEFAULT_HEX_REWARD_TYPE;

extern int DEFAULT_NUM_STATES;

extern int DEFAULT_NUM_SIMULATIONS;
extern int DEFAULT_MAX_DEPTH;

extern bool DEFAULT_SAMPLE_ACTIONS;
extern bool DEFAULT_REQUIRES_NN;
extern bool DEFAULT_USE_RAVE;

extern double DEFAULT_C_B;
extern double DEFAULT_C_RAVE;
extern double DEFAULT_W_A;

extern int DEFAULT_MINIBATCH_SIZE;
extern int DEFAULT_NUM_THREADS;
extern int DEFAULT_LOG_EVERY;

extern int DEFAULT_STATES_PER_FILE;


#endif
