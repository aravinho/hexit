import sys
from utils import *
from config import *





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
    --model_spec <the file where the model_spec is saved> [required] \


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
    --model_spec models/hex/5/best_model/spec.json \

    """

    # Parse and verify arguments
    arg_map = parseArgs(sys.argv[1:])

    training_data_path = arg_map.getString("training_data_path", required=True)
    print "training_data_path: ", training_data_path

    model_path = arg_map.getString("model_path", required=True)
    print "model_path: ", model_path

    model_spec = arg_map.getString("model_spec", required=True)
    print "model_spec: ", model_spec

    dataset_size = arg_map.getInt("dataset_size", default_val=DEFAULT_DATASET_SIZE)
    print "dataset_size: ", dataset_size

    batch_size = arg_map.getInt("batch_size", default_val=DEFAULT_BATCH_SIZE)
    print "batch_size: ", batch_size

    num_epochs = arg_map.getInt("num_epochs", default_val=DEFAULT_NUM_EPOCHS)
    print "num_epochs: ", num_epochs

    learning_rate = arg_map.getFloat("learning_rate", default_val=DEFAULT_LEARNING_RATE)
    print "learning_rate: ", learning_rate

    from_scratch = arg_map.getBoolean("from_scratch", default_val=DEFAULT_FROM_SCRATCH)
    print "from_scratch: ", from_scratch

    log_every = arg_map.getInt("log_every", default_val=DEFAULT_LOG_EVERY_BATCH)
    print "log_every: ", log_every

    save_every = arg_map.getInt("save_every", default_val=DEFAULT_SAVE_EVERY)
    print "save_every: ", save_every

    # model = ExitNN(model_spec)
    # model.train(training_data_path, dataset_size=dataset_size, batch_size=batch_size, num_epochs=num_epochs, 
    #   learning_rate=learning_rate, from_scratch=from_scratch, log_every=log_every, save_every=save_every)





if __name__ == '__main__':
    main()