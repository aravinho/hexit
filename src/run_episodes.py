import sys
import os
import datetime, time
import tensorflow as tf

from agents import *
from utils import *
from config import *



hex_board_weights = {5: list(range(26))}



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
    Returns a tuple of (game_states, reward, winner), where game_states is a list of game states seen
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


    # If either player is an agent, begin a TF session and restore the NN models
    
    p1_nn = type(p1_agent) == NNAgent
    p2_nn = type(p2_agent) == NNAgent

    with tf.Session() as sess:

        if p1_nn:
            p1_agent.restoreModel(sess)
        if p2_nn:
            p2_agent.restoreModel(sess)

        while not state.isTerminalState():
            if display_state:
                print
                state.printBoard()
                print

            game_states.append(state)

            if state.turn() == 1:
                if p1_nn:
                    action = p1_agent.getAction(state, sess)
                else:
                    action = p1_agent.getAction(state)

            elif state.turn() == -1:
                if p2_nn:
                    action = p2_agent.getAction(state, sess)
                else:
                    action = p2_agent.getAction(state)

            else:
                assert False, "Turn must be either 1 or -1"

            state = state.nextState(action)

    if display_state:
        state.printBoard()
        print

    reward = state.reward()
    winner = state.winner()
    if display_state:
        declareWinner(winner, reward)

    return game_states, reward, winner


def chooseRandomState(states, weights=None):
    """
    Chooses a state at random from the given list.
    If WITH_WEIGHT is set, sample proportional to these weights.
    """
    if weights is None:
        return random.choice(states)

    return weightedSample(weights, vals=states)





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




def runNNEpisodes(game, game_specs, num_episodes, p1_agent, p2_agent, batch_size=DEFAULT_BATCH_SIZE, log_every=DEFAULT_LOG_EVERY):
    """
    Sped-up version of batching NN inference to make episodes of NN vs NN run faster.
    Plays NUM_EPISODES episodes of P1_AGENT vs P2_AGENT.
    Returns one episode from each episode, sampled proportional to the weight of the board.
    """

    if game == "hex":
        assert "dimension" in game_specs, "dimension not given in game_specs in playGame function"
        dimension = game_specs["dimension"]
        assert dimension > 0, "dimension must be greater than 0"
        termination_weights = [i + 1 for i in range(dimension ** 2 + 1)]
        termination_weights = [w / float(sum(termination_weights)) for w in termination_weights]

    # Keep track of one state per episode, to return
    sampled_states = [None for i in range(num_episodes)]

    # Initialize the NN Batch. this will be updated within the loop
    dummy_state = HexState(dimension, [0 for pos in range(dimension ** 2)])
    state_vector_size = len(dummy_state.makeStateVector())
    state_vector_batch = np.zeros((batch_size, state_vector_size))

    # Determine the number of batches
    num_batches = num_episodes // batch_size
    if num_batches > 0 and num_episodes % num_batches > 0:
        num_batches += 1

    num_total_finished = 0
        
    # Start the TF session
    with tf.Session() as sess:

        # Initialize both models
        p1_agent.restoreModel(sess)
        p2_agent.restoreModel(sess)

        # Break into batches, and run episodes for one batch at a time
        for batch_num in range(0, num_batches):

            # Keep track of all states in these episodes to make sure we don't save terminal states
            episodes = [[None for i in range(dimension ** 2 + 1)] for j in range(batch_size)]
            for ep in range(batch_size):
                episodes[ep][0] = HexState(dimension, [0 for pos in range(dimension ** 2)])

            # Initialize a fresh new batch of empty boards
            active_states = [HexState(dimension, [0 for pos in range(dimension ** 2)]) for i in range(batch_size)]
            # Keep track of finished episodes
            finished_episodes = [False for i in range(batch_size)]
            num_finished_episodes = batch_size
            # Determine after how many states the episode will artificially terminate
            terminate_at = [weightedSample(termination_weights) for i in range(batch_size)]
            # initialize the turn to 1
            turn = 1

            # Repeat until all the episodes in this batch are over
            num_moves = 0
            while num_finished_episodes > 0:
            
                # for every active episode in this batch, submit the state vector to the batch
                for batch_index in range(batch_size):
                    episode_num = (batch_num * batch_size) + batch_index

                    if episode_num >= num_episodes:
                        continue

                    # If this state is over, or is determined to terminate at this point, mark it as terminated and save the state
                    state = active_states[batch_index]
                    # Add this state to the array of episodes
                    episodes[batch_index][num_moves] = state

                    if (state.isTerminalState() or num_moves == terminate_at[batch_index]) and not finished_episodes[batch_index]:
                        finished_episodes[batch_index] = True
                        num_finished_episodes -= 1
                        num_total_finished += 1
                        if num_total_finished % log_every == 0:
                            writeLog("Finished " + str(num_total_finished) + " episodes")

                        # make sure we don't save a terminal state
                        if state.isTerminalState():
                            sampled_states[episode_num] = episodes[batch_index][num_moves - 1]
                        else:
                            sampled_states[episode_num] = state
                        
                        
                        continue

                    # submit the state vector, and advance to the next state
                    state_vector_batch[batch_index] = state.makeStateVector()


                # Submit the state vector batch to the NN, via the NNAgent
                if turn == 1:
                    actions = p1_agent.predictBatch(state_vector_batch, sess)
                if turn == -1:
                    actions = p2_agent.predictBatch(state_vector_batch, sess)

                
                # Advance all the active episodes by taking the action. If an episode is finished, mark it so.
                for batch_index in range(batch_size):
                    episode_num = (batch_num * batch_size) + batch_index
                    if episode_num >= num_episodes or finished_episodes[batch_index]:
                        continue

                    # Advance to the next state, make sure action is legal
                    state = active_states[batch_index]
                    action = actions[batch_index]
                    if state.isLegalAction(action):
                        active_states[batch_index] = state.nextState(action)
                    else:
                        active_states[batch_index] = state.nextState(state.randomAction())


                    

                # Update the turn and number of moves that have occurred in this batch of episodes
                turn *= -1
                num_moves += 1

    return sampled_states






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
    --p1_agent_sample True [include if the NN should sample actions; leave out for argmax] \
    \
    --p2_agent <nn OR user OR random> [defaults to user] \
    --p2_model_dir <path to NN model to be used for Agent 2> [unnecessary for non-NN Player 2 Agents] \
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
    --p1_agent_sample True
    \
    --p2_agent nn \
    --p2_model_path models/hex/5/best_model/ \
    \
    --random_first_move 0.25 \
    \
    --log_every 512 \
    --save_path data/nn_states/hex/5/best_model \
    --states_per_file 1024

    Assuming that the directory data/nn_states/hex/5/best_model is empty, this run will write the files
    0.csv, 1.csv, 2.csv, and 3.csv into that directory.

    """

    # Turn off tensorflow warnings
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

    # Parse and verify arguments
    arg_map = parseArgs(sys.argv[1:])


    # Game
    game = arg_map.getString("game", default_val=DEFAULT_GAME)
    assert game in GAME_OPTIONS, "Game " + game + " not supported"

    # Number of episodes
    num_episodes = arg_map.getInt("num_episodes", default_val=DEFAULT_NUM_GAMES)
    assert num_episodes >= 0, "Cannot have a negative number of episodes"

    # Dimension of hex game
    hex_dim = arg_map.getInt("hex_dim", default_val=DEFAULT_HEX_DIM)
    assert hex_dim > 0, "Must have a positive hex dim"

    # Player 1 agent
    p1_agent_type = arg_map.getString("p1_agent", default_val=DEFAULT_P1_AGENT)
    assert p1_agent_type in AGENT_TYPES, "Agent type " + p1_agent_type + " not supported"

    if p1_agent_type == "nn":
        p1_model_path = arg_map.getString("p1_model_path", required=True)
        p1_agent_sample = arg_map.getBoolean("p1_agent_sample", default_val=DEFAULT_P1_AGENT_SAMPLE)
        

    # Player 2 agent
    p2_agent_type = arg_map.getString("p2_agent", default_val=DEFAULT_P2_AGENT)
    assert p2_agent_type in AGENT_TYPES, "Agent type " + p2_agent_type + " not supported"

    if p2_agent_type == "nn":
        p2_model_path = arg_map.getString("p2_model_path", required=True)
        p2_agent_sample = arg_map.getBoolean("p2_agent_sample", default_val=DEFAULT_P2_AGENT_SAMPLE)

    # Random first move?
    random_first_move = arg_map.getFloat("random_first_move", default_val=DEFAULT_RANDOM_FIRST_MOVE)
    assert 0.0 <= random_first_move <= 1.0, "Invalid random_first_move argument " + str(random_first_move)

    # Display board after every move?
    display_state = arg_map.getBoolean("display_state", default_val=DEFAULT_DISPLAY_STATE)

    # Log every F iterations
    log_every = arg_map.getInt("log_every", default_val=DEFAULT_LOG_EVERY)
    if arg_map.contains("save_path"):
        save_path = arg_map.getString("save_path", required=True)
    states_per_file = arg_map.getInt("states_per_file", default_val=DEFAULT_STATES_PER_FILE)


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
        if game == "hex":
            p1_agent = NNAgent(HexNN(dim=hex_dim), p1_model_path, sample=p1_agent_sample)


    # Create Agent 2
    if p2_agent_type == "user":
        p2_agent = UserAgent(game)
    elif p2_agent_type == "random":
        p2_agent = RandomAgent(game)
    elif p2_agent_type == "nn":
        if game == "hex":
            p2_agent = NNAgent(HexNN(dim=hex_dim), p2_model_path, sample=p2_agent_sample)


    # Run episodes

    writeLog("Running " + str(num_episodes) + " episodes")


    # If both are NN, run the batch version
    if p1_agent_type == "nn" and p2_agent_type == "nn":
        
        batch_size = arg_map.getInt("batch_size", default_val=DEFAULT_BATCH_SIZE)
        
        # Break the work up by file
        num_files = num_episodes // states_per_file
        
        if num_files == 0:
            generated_states = runNNEpisodes(game, game_specs, num_episodes, p1_agent, p2_agent, batch_size=batch_size, log_every=log_every)
            # save states to file
            if arg_map.contains("save_path"):
                writeLog("Saving states to path " + save_path)
                writeStatesToFile(generated_states, save_path)


        for file_num in range(num_files):
            episodes_left = num_episodes - (file_num * states_per_file)
            generated_states = runNNEpisodes(game, game_specs, min(episodes_left, states_per_file),
                p1_agent, p2_agent, batch_size=batch_size, log_every=log_every)
            
            # save states to file
            if arg_map.contains("save_path"):
                writeLog(str(file_num) + ", Saving states to path " + save_path)
                writeStatesToFile(generated_states, save_path)
        
        writeLog("Ran " + str(num_episodes) + " episodes.")
        
        exit()




    # If only one (or none) of the players are an NN, run simple episodes

    player_1_wins, player_2_wins, draws = 0, 0, 0

    for episode_num in range(num_episodes):

        if episode_num % log_every == 0:
            print "Running episode", episode_num

        batch_num = episode_num % states_per_file

        # save states to file if necessary
        if batch_num == 0 and episode_num > 0 and arg_map.contains("save_path"):
            print "Saving states to path", save_path
            writeStatesToFile(generated_states, save_path)
            

        # If this is a new batch, reinitialize the in-memory buffer
        if batch_num == 0: 
            generated_states = [None for episode_num in range(min(num_episodes, states_per_file))]

        # Run an episode, choose a random state from it, and store that state in the buffer
        episode_states, reward, winner = runEpisode(game, p1_agent, p2_agent, game_specs, random_first_move, display_state)


        random_state = chooseRandomState(episode_states, weights = hex_board_weights[hex_dim])
        generated_states[batch_num] = random_state
        if winner == 1:
            player_1_wins += 1
        elif winner == -1:
            player_2_wins += 1
        elif winner == 0:
            draws += 1

    print "Player 1 wins:", player_1_wins
    print "Player 2 wins:", player_2_wins
    print "Draws:", draws

    writeLog("Finished running " + str(num_episodes))



    # Save any remaining states
    if arg_map.contains("save_path"):
        print "Saving states to path", save_path
        writeStatesToFile(generated_states, save_path)
       






   



if __name__ == '__main__':
    main()