#ifndef MAIN_H
#define MAIN_H

//#include "mcts_thread_manager.h"
#include "mcts.h"
#include <vector>

/**
 * Parses command line arguments into a map of "key-value" pairs.
 * The arguments are expected to be in pairs.  The "key" is of the format --option, and the value is another string.
 * For example, the given array of char-pointer args may look like:
 *
 * ["--game", "hex", "--hex_dim", "5"]
 * 
 * This function would populate the map STR_ARGS (which is expected to be empty initially) to look like {"game" --> "hex", "hex_dim" --> "5"}
 * This function begins parsing arguments at the given START_INDEX.  This defaults to 1, because the first command line argument is
 * usually the name of the executable.
 * The first argument num_args gives the number of elements in the entire argv array (starting from element 0)
 *
 * 
 * Errors if there are an odd number of elements in args, or if any of the "keys" do not start with "--".
 */
void parseArgs(int argc, char* argv[], ArgMap* arg_map, int start_index=1);


/**
 * Worker thread function (for thread THREAD_NUM) that runs vanilla MCTS (without an NN apprentice).
 * This function runs MCTS from the root node states given in the NODES vector.
 * It is responsible for nodes starting at index START, up till but not including index END.
 * For every one of these nodes, runs simulations to a max_depth of MAX_DEPTH, and logs after the completion of
 * every batch of LOG_EVERY nodes.
 */
void threadFunc(int thread_num, vector<MCTS_Node*>* nodes, int start, int end, int max_depth=DEFAULT_MAX_DEPTH, int log_every=DEFAULT_LOG_EVERY);


/** 
 * Runs MCTS Simulations, 
 * Usage (use "make" to compile first):

 ./bin/run-mcts \
 	--game <name of game> [optional - defaults to "hex"] \
 	--hex_dim <dimension of hex board> [only applicable if the game is hex; defaults to 5] \
 	\
 	--num_states <number of states on which to run simulations> [optional - defaults to 4096] \
 	--input_data_path <path of directory where states representations are stored> [required] \
 	\
 	--num_simulations <number of simulations to run for each state> [optional - defaults to 1000] \
 	--max_depth <max depth of MCTS trees> [optional - defaults to 4] \
 	\
 	--minibatch_size <size of minibatches used for multi-threading apprentice querying> [optional - defaults to 256] \
 	--num_threads <number of worker threads in multi-threaded MCTS> [optional - defaults to 4] \
 	--log_every <logs every time this many nodes finish> [optional - defaults to 512] \
    \
    --use_nn <True if you want to query a NN apprentice> [optional - if omitted or given a value other than "True", no NN is used] \
    --model_spec <the path at which the model spec file is stored> [required if use_nn is True] \
    --model_path <the path at which the saved model is stored> [required if use_nn is True] \
    --nn_script <the name of the script used to query the NN [required if use_nn is True] \
    \
    --output_data_path <the path to which the states and resulting action distributions will be saved [optional - if not provided, not saved] \
    --states_per_file <the number of states to which to save to each file> [optional - defaults to 2^20] \


    * Note: the input_data_path must specify a directory that contains .csv files, indexed by numbers (0.csv, 1.csv, etc).
    Data will be read from these in order, until the appropriate number of states is read.
    Similarly, output_data_path specifies a directory.  The states are written to .csv files in output_data_path/states,
    and the associated action distributions are written to .csv files in output_data_path/action_distributions.
    STATES_PER_FILE states are written per file.
    Suppose the output_data_path directory is initially empty.  Then this will write first to 0.csv, and if necessary, then to 1.csv, 2.csv, etc.
    If the output_data_path directory already contains up to <k>.csv, this will begin writing at <k + 1>.csv.

 Example Usage without NN:

 ./bin/run-mcts \
 	--game hex \
 	--hex_dim 5 \
 	\
 	--num_states 4096 \
 	--input_data_path data/nn_states/hex/5/best_model/0/ \
 	\
 	--num_simulations 1000 \
 	--max_depth 4 \
 	\
 	--minibatch_size 256 \
 	--num_threads 4 \
 	--log_every 512 \
    \
    --output_data_path data/mcts/hex/5/best_model/ \
    --states_per_file 1024 \

 Example Usage with NN:

 ./bin/run-mcts \
 	--game hex \
 	--hex_dim 5 \
 	\
 	--num_states 4096 \
 	--input_data_path data/nn_states/hex/5/best_model/1 \
 	\
 	--num_simulations 1000 \
 	--max_depth 4 \
 	\
 	--minibatch_size 256 \
 	--num_threads 4 \
 	--log_every 512 \
    \
    --use_nn True \
    --model_spec models/hex/5/best_model/spec.json \
    --model_path models/hex/5/best_model/ \
    --nn_script src/nn_query.py \
    \
    --output_data_path data/mcts/hex/5/best_model/ \
    --states_per_file 1024 \
 
 *
 */
int main(int argc, char *argv[]);

#endif