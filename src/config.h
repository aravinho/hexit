#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <set>
#include <math.h>

using namespace std;


/* Default config options */

extern set<string> GAME_OPTIONS; // what games (environments) are supported

extern string DEFAULT_GAME; // default game (hex)
extern int DEFAULT_HEX_DIM; // default dimension of hex board (5)
extern string DEFAULT_HEX_REWARD_TYPE; // default reward type for hex (win_fast)

extern int DEFAULT_NUM_STATES; // default number of states on which to run MCTS (1024)

extern int DEFAULT_NUM_SIMULATIONS; // default number of simulations to run for each state (1024)
extern int DEFAULT_MAX_DEPTH; // default depth to expand the MCTS tree before performing rollout (4)

extern bool DEFAULT_SAMPLE_ACTIONS; // the default of whether MCTS should sample actions proportional to scores or act greedily (True; samples actions)
extern bool DEFAULT_REQUIRES_NN; // the default of whether MCTS is bootstrapped with a neural network apprentice (False)
extern bool DEFAULT_USE_RAVE; // the default of whether MCTS uses Rapid Value Estimation (RAVE)

extern double DEFAULT_C_B; // the default for the hyperparameter that weighs MCTS exploration vs exploitation (0.05)
extern double DEFAULT_C_RAVE; // the default for the hyperparameter that governs how fast RAVE is downweighted as the number of samples increase (3000)
extern double DEFAULT_W_A; // the default for the hyperparameter that weighs the apprentice (NN) predictions against MCTS (40)

extern int DEFAULT_MINIBATCH_SIZE; // the default minibatch size for querying the NN apprentice (128)
extern int DEFAULT_NUM_THREADS; // the default number of worker threads to use to parallelize MCTS (4)
extern int DEFAULT_LOG_EVERY; // the default number of states after which to log (512)

extern int DEFAULT_STATES_PER_FILE; // the default number of states to write to each output file (1024)
extern int DEFAULT_START_AT; // the default file number from which to start reading states (0; means start reading from 0.csv in the given data directory)



#endif
