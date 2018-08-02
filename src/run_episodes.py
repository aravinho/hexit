import sys
import os
from agents import *
from utils import *
from config import *






def runEpisode(game, p1_agent, p2_agent, game_specs, random_first_move=0.0, display_state=False):
    """
    Plays through a single game.
    The game is given by the string argument "game" ("tictactoe" or "hex")
    The Player 1 and Player 2 agents are given by the arguments p1_agent and p2_agent,
    both of which must be subclasses of GameAgent.
    The game_specs argument is a dictionary that specifies any necessary details of the game
    (for example, the dimension of the hex state)
    The random_first_move flag specifies whether the first move taken should be random.
    The display_state flag specifies whether to display the state between moves.
    (This is useful for playing games where one or both agents are users.)
    """
    
    if game == "tictactoe":
        state = Tictactoe([0,0,0,0,0,0,0,0,0])
        
    elif game == "hex":
        assert "dimension" in game_specs, "dimension not given in game_specs in playGame function"
        dimension = game_specs["dimension"]
        assert dimension > 0, "dimension must be greater than 0"
        state = HexState(dimension, [0 for pos in range(dimension ** 2)])

    game_states = []

    if decision(random_first_move):
        game_states.append(state)
        random_action = state.randomAction()
        state = state.nextState(random_action)


    while not state.isTerminalState():
        if display_state:
            print
            state.printBoard()
            print

        game_states.append(state)

        if state.turn() == 1:
            action = p1_agent.getAction(state)

        if state.turn() == -1:
            action = p2_agent.getAction(state)


        state = state.nextState(action)

    if display_state:
        state.printBoard()
        print

    reward = state.reward()
    winner = state.winner()
    if display_state:
        declareWinner(winner, reward)

    return game_states





def writeStatesToFile(states, save_path):
    """
    Writes the states in the given buffer to the next available CSV file in the given directory.
    For example, if the directory given by save_path already contains 0.csv and 1.csv, this call will write to 2.csv.
    """

    assert len(save_path) > 0, "Must have nonempty save path"
    if save_path[-1] != '/':
        save_path += '/'

    # make directory if necessary
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    # grab all existing csv files
    all_files = [f for f in os.listdir(save_path) if os.path.isfile(os.path.join(save_path, f))]
    csv_files = [f for f in all_files if len(f) > 4 and f[-4:] == ".csv"]
    csv_indices = [int(f[0:-4]) for f in csv_files]
    csv_indices.sort()
    
    # determine the new filename to which we will write
    new_file_index = 0
    if len(csv_indices) > 0:
        new_file_index = csv_indices[-1] + 1
    filename = save_path + str(new_file_index) + ".csv"

    # write the states to file
    f = open(filename, 'w')
    for state in states:
        f.write(state.asCSVString())
        f.write("\n")
    f.close()

    pass


def main():
    """
    Usage:

    python src/run_episodes.py \
    --game <game_name> [defaults to "hex"] \
    --num_episodes <number of games> [defaults to 1] \
    --hex_dim <dimension of hex game> [unnecessary for non-hex games, defaults to 5] \
    \
    --p1_agent <nn OR user OR random> [defaults to user] \
    --p1_model_path <path to NN model to be used for Agent 1> [unnecessary for non-NN Player 1 Agents] \
    --p1_model_spec <path to JSON spec file for Agent 1 NN Model> [unnecessary for non-NN Player 1 Agents] \
    --p1_agent_sample True [include if the NN should sample actions; leave out for argmax] \
    \
    --p2_agent <nn OR user OR random> [defaults to user] \
    --p2_model_dir <path to NN model to be used for Agent 2> [unnecessary for non-NN Player 2 Agents] \
    --p2_model_spec <path to JSON spec file for Agent 2 NN Model> [unnecessary for non-NN Player 2 Agents] \
    --p2_agent_sample True [include if the NN should sample actions; leave out for argmax] \
    \
    --random_first_move <probability with which to randomly choose a first move> [optional, defaults to 0.0] \
    --display_state True [include if state should be displayed after each action; if not included, or given any value other than True, defaults to False ] \
    \
    --log_every <logging frequency F> [optional, prints a message after F episodes are run; defaults to 10] \
    --save_path <path to directory in which random states (one per episode) should be saved as a CSV file> [optional] \
    --states_per_file <number of states which each saved file should contain> [unnecessary without save_path; defaults to 2**20] \

    Note: The save_path argument specifies a directory.  This script will write one or more .csv files into this directory.    
    If the directory already contains, say, 0.csv and 1.csv, this script will write files indexed starting at 2.csv.


    Example:

    python src/run_episodes.py \
    --game hex \
    --num_episodes 4096 \
    --hex_dim 5 \
    \
    --p1_agent nn \
    --p1_model_path models/hex/5/best_model/ \
    --p1_model_spec models/hex/5/best_model/spec.json \
    --p1_agent_sample True
    \
    --p2_agent nn \
    --p2_model_path models/hex/5/best_model/ \
    --p2_model_spec models/hex/5/best_model/spec.json \
    \
    --random_first_move 0.25 \
    \
    --log_every 512 \
    --save_path data/nn_states/hex/5/best_model \
    --states_per_file 1024

    Assuming that the directory data/nn_states/hex/5/best_model is empty, this run will write the files
    0.csv, 1.csv, 2.csv, and 3.csv into that directory.

    """

    # Parse and verify arguments
    args = parseArgs(sys.argv[1:])

    # Game
    game = args["game"] if "game" in args else DEFAULT_GAME
    assert game in GAME_OPTIONS, "Invalid game argument " + game

    # Number of episodes
    num_episodes = int(args["num_episodes"]) if "num_episodes" in args else DEFAULT_NUM_GAMES
    assert num_episodes >= 0, "Invalid num_episodes argument " + str(num_episodes)

    # Dimension of hex game
    hex_dim = int(args["hex_dim"]) if "hex_dim" in args else DEFAULT_HEX_DIM
    assert hex_dim > 0, "Invalid hex_dim argument " + str(hex_dim)

    # Player 1 agent
    p1_agent_type = args["p1_agent"] if "p1_agent" in args else DEFAULT_P1_AGENT
    assert p1_agent_type in AGENT_TYPES, "Invalid p1_agent_type argument " + p1_agent_type
    if p1_agent_type == "nn":
        assert "p1_model_path" in args, "If Player 1 agent is an NN agent, must specify model path"
        p1_model_path = args["p1_model_path"]
        assert "p1_model_spec" in args, "If Player 1 agent is an NN agent, must specify model spec"
        p1_model_spec = args["p1_model_spec"]
        p1_agent_sample = DEFAULT_P1_AGENT_SAMPLE
        if "p1_agent_sample" in args and args["p1_agent_sample"] == "True":
            p1_agent_sample = True

    # Player 2 agent
    p2_agent_type = args["p2_agent"] if "p2_agent" in args else DEFAULT_P2_AGENT
    assert p2_agent_type in AGENT_TYPES, "Invalid p2_agent_type argument " + p2_agent_type
    if p2_agent_type == "nn":
        assert "p2_model_path" in args, "If Player 2 agent is an NN agent, must specify model path"
        p2_model_path = args["p2_model_path"]
        assert "p2_model_spec" in args, "If Player 2 agent is an NN agent, must specify model spec"
        p2_model_spec = args["p2_model_spec"]
        p2_agent_sample = DEFAULT_P2_AGENT_SAMPLE
        if "p2_agent_sample" in args and args["p2_agent_sample"] == "True":
            p2_agent_sample = True

    # Random first move?
    random_first_move = float(args["random_first_move"]) if "random_first_move" in args else DEFAULT_RANDOM_FIRST_MOVE
    assert 0.0 <= random_first_move <= 1.0, "Invalid random_first_move argument " + str(random_first_move)

    # Display board after every move?
    display_state = DEFAULT_DISPLAY_STATE
    if "display_state" in args and args["display_state"] == "True":
        display_state = True

    # Log every F iterations
    log_every = int(args["log_every"]) if "log_every" in args else DEFAULT_LOG_EVERY
    if "save_path" in args:
        save_path = args["save_path"]
    states_per_file = int(args["states_per_file"]) if "states_per_file" in args else DEFAULT_STATES_PER_FILE


    # Create Game specs
    game_specs = {}
    if game == "hex":
        game_specs = {"dimension": hex_dim}

    # Create Agent 1
    if p1_agent_type == "user":
        p1_agent = UserAgent(game)
    elif p1_agent_type == "random":
        p1_agent = RandomAgent(game)
    elif p1_agent_type == "nn":
        p1_agent = NNAgent(game=game, model_spec=p1_model_spec, model_path=p1_model_path, sample=p1_agent_sample)


    # Create Agent 2
    if p2_agent_type == "user":
        p2_agent = UserAgent(game)
    elif p2_agent_type == "random":
        p2_agent = RandomAgent(game)
    elif p2_agent_type == "nn":
        p2_agent = NNAgent(game=game, model_spec=p2_model_spec, model_path=p2_model_path, sample=p2_agent_sample)


    # Run episodes
    for episode_num in range(num_episodes):

        if episode_num % log_every == 0:
                print "Running episode", episode_num

        batch_num = episode_num % states_per_file

        # If this is a new batch, reinitialize the in-memory buffer
        if batch_num == 0: 
            generated_states = [None for episode_num in range(min(num_episodes, states_per_file))]

        # Run an episode, choose a random state from it, and store that state in the buffer
        episode_states = runEpisode(game, p1_agent, p2_agent, game_specs, random_first_move, display_state)
        random_state = random.choice(episode_states)
        generated_states[batch_num] = random_state

        # save states to file if necessary
        if batch_num + 1 == states_per_file and "save_path" in args:
                print "Saving states to path", save_path
                writeStatesToFile(generated_states, save_path)
            






   



if __name__ == '__main__':
    main()