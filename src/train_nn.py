from utils import *
from config import *
from exit_nn import ExitNN
from hex_nn import HexNN

import sys
import os

def main():
    """
    Usage:

    python src/train_nn.py \
    --game <game_name> [defaults to "hex"] \
    --hex_dim <dimension of hex game> [unnecessary for non-hex games, defaults to 3] \
    \
    --training_data_path <path where training data (states and action distributions are stored> [required] \
    --begin_from <file number from which to begin reading data (assumes ordered file names)> [optional; defaults to 0] \
    --dataset_size <total size of dataset> [optional; defaults to 4096] \
    --batch_size <size of training batch> [optional; defaults to 256] \
    --num_epochs <number of epochs to train for> [optional; defaults to 5] \
    --log_every <log after this many minibatches> [optional; defaults to 8] \
    --save_every <save every this many epochs> [optional; defaults to 1] \
    --learning_rate [optional; defaults to 0.01] \
    --from_scratch <whether to begin training the model from scratch or restore from a checkpoint [optional; defaults to True] \
    \
    --model_path <the directory where the model is saved (or will be saved) [required] \


    Example:

    python src/train_nn.py \
    --game hex \
    --hex_dim 5 \
    \
    --training_data_path data/mcts/hex/5/best_model/ \
    --begin_from 0 \
    --dataset_size 4096 \
    --batch_size 256 \
    --num_epochs 5 \
    --log_every 8 \
    --save_every 1 \
    --learning_rate 0.01 \
    --from_scratch True \
    \
    --model_path models/hex/5/best_model/ \

    """

    # Parse and verify arguments
    arg_map = parseArgs(sys.argv[1:])

    game = arg_map.getString("game", default_val=DEFAULT_GAME)

    hex_dim = arg_map.getInt("hex_dim", default_val=DEFAULT_HEX_DIM)

    training_data_path = arg_map.getString("training_data_path", required=True)

    model_path = arg_map.getString("model_path", required=True)

    new_model_path = arg_map.getString("new_model_path", default_val=model_path)

    begin_from = arg_map.getInt("begin_from", default_val=DEFAULT_BEGIN_FROM)

    dataset_size = arg_map.getInt("dataset_size", default_val=DEFAULT_DATASET_SIZE)

    split = arg_map.getFloat("split", default_val=DEFAULT_TRAIN_VAL_SPLIT)

    batch_size = arg_map.getInt("batch_size", default_val=DEFAULT_BATCH_SIZE)

    num_epochs = arg_map.getInt("num_epochs", default_val=DEFAULT_NUM_EPOCHS)

    learning_rate = arg_map.getFloat("learning_rate", default_val=DEFAULT_LEARNING_RATE)
    print "learning rate: ", learning_rate

    from_scratch = arg_map.getBoolean("from_scratch", default_val=DEFAULT_FROM_SCRATCH)
    print "from_scratch", from_scratch

    log_every = arg_map.getInt("log_every", default_val=DEFAULT_LOG_EVERY_BATCH)

    save_every = arg_map.getInt("save_every", default_val=DEFAULT_SAVE_EVERY)


    # Turn off tensorflow warnings
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

    model = HexNN(hex_dim)

    writeLog('Beginning training dataset of size ' + str(dataset_size))
    model.train(training_data_path, model_ckpt_dir=model_path, new_model_ckpt_dir=new_model_path, from_scratch=from_scratch, dataset_size=dataset_size, split=split, begin_from=begin_from,
        num_epochs=num_epochs, batch_size=batch_size, learning_rate=learning_rate, log_every=log_every, save_every=save_every)
    writeLog('Done training dataset of size ' + str(dataset_size))





if __name__ == '__main__':
    main()