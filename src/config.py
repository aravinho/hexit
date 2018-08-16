"""
Default values for used by various parts of the system.
"""

DEFAULT_GAME = "hex" # Default game to run episodes for/train model for
DEFAULT_NUM_GAMES = 1 # Default number of episodes to run
DEFAULT_HEX_DIM = 5 # Default dimension of Hex board
DEFAULT_P1_AGENT = "user" # Default Player 1 Agent for Episodes
DEFAULT_P2_AGENT = "user" # Default Player 2 Agent for Episodes
DEFAULT_P1_AGENT_SAMPLE = True # If the Player 1 Agent is an NN, whether it samples (or uses argmax)
DEFAULT_P2_AGENT_SAMPLE = True # If the Player 2 Agent is an NN, whether it samples (or uses argmax)

DEFAULT_RANDOM_FIRST_MOVE = 0.0 # The probability with which the first move of an episode should be made at random
DEFAULT_DISPLAY_STATE = False # Whether the state (board) should be displayed while running episodes
DEFAULT_LOG_EVERY = 10 # Log after this many episodes
DEFAULT_SAVE_PATH = True # whether to save states created
DEFAULT_STATES_PER_FILE = 1024 # How many states to store per file
 
GAME_OPTIONS = ["tictactoe", "hex"] # The list of supported "games" (environments)
AGENT_TYPES = ["nn", "user", "random"] # The list of supported agent types

DEFAULT_DATASET_SIZE = 4096 # The default size of a training dataset
DEFAULT_BATCH_SIZE = 256 # The default size of one NN training batch
DEFAULT_NUM_EPOCHS = 5 # The default number of training epochs to run
DEFAULT_LOG_EVERY_BATCH = 8 # The default number of training batches after which to log
DEFAULT_SAVE_EVERY = 1 # The default number of epochs after which to save the model
DEFAULT_LEARNING_RATE = 0.01 # The default learning rate for training
DEFAULT_FROM_SCRATCH = True # Whether to begin training a model from scratch, or load a saved model

DEFAULT_BEGIN_FROM = 0 # The default CSV file in a directory to begin reading from
DEFAULT_MAX_ROWS = 2**20 # The default number of CSV rows to read from a directory that contains CSV files
DEFAULT_TRAIN_VAL_SPLIT = 0.85 # The proportion of training data that should be used for training (the rest is for validation)

DEFAULT_SAMPLE_PREDICTION = True # The default of whether the NN should sample an action in predictSingle (or use argmax)
