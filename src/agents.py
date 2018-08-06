from exit_nn import *
from hex import HexState
from hex_nn import HexNN


class GameAgent(object):
    """
    The superclass of all Agent classes.
    All inheriting classes must implement a getAction method.
    Given a state, this method returns the index of the chosen action.
    """
    def __init__(self, game):
        """
        The "game" argument is a string that specifies what game this agent is playing.
        """
        self.game = game

    def getAction(self, state):
        """
        Returns a legal action for the given state.
        """
        return -1



class NNAgent(GameAgent):
    """ 
    The NNAgent class chooses actions by having a neural network predict optimal actions for the given state.
    """

    def __init__(self, nn, model_path, sample=True):
        """
        The nn argument specifies the NN that will be choosing actions.
        The model_path argument allows us to restore a model from a checkpoint file.
        The sample argument tells the NN whether to sample the best action.
        If false, it just chooses the action with the highest score.
        """
        self.model_path = model_path
        self.sample = sample
        self.nn = nn
        self.nodes = None # will be set by restoreModel

    def restoreModel(self, sess):
        """
        Restores this agent's NN model from disk, and stores the output node, so it can quickly run forward pass inference
        """
        self.nn.restoreCheckpoint(sess, self.model_path)
        self.nodes = self.nn.getFromCollection()


    def predictBatch(self, states, sess):
        """
        Given a batch of state vectors.
        Makes predictions for them.
        Returns an array of chosen actions.
        """
        num_states = states.shape[0]
        chosen_actions = self.nn.predict(states, sess, self.model_path, nodes=self.nodes, sample=self.sample)
        return chosen_actions


    def getAction(self, state, sess):
        """
        Returns a legal action for the given state. The action is chosen by the NN.
        If the NN keeps repeatedly suggesting illegal actions, eventually error.
        """

        # create np array, feed to predict method
        state_vector = state.makeStateVector()
        chosen_action = self.nn.predict(state_vector, sess, self.model_path, nodes=self.nodes, sample=self.sample)
        if not state.isLegalAction(chosen_action):
            chosen_action = state.randomAction()

        return chosen_action


class RandomAgent(GameAgent):
    """
    The RandomAgent class chooses actions uniformly at random from the set of legal actions.
    """

    def __init__(self, game):
        self.game = game

    def getAction(self, state):
        """
        Returns a legal action for the given state, chosen uniformly at random from the set of legal actions.
        """
        return state.randomAction()




class UserAgent(GameAgent):
    """
    The UserAgent class chooses actions by prompting a user to select one.
    """

    def __init__(self, game):
        self.game = game


    def getAction(self, state):
        """
        Returns the action chosen by the user for the given state.
        If the user chooses an illegal action, prompt again.
        If the user repeatedly chooses illegal actions, error.
        """

        chosen_action = -1
        legal_action = False
        t = 0
        while not legal_action and t < 10:
            chosen_action = input("Enter a legal action number: ")
            legal_action = state.isLegalAction(chosen_action)
            if not legal_action:
                print "Action", chosen_action, "is illegal.  Choose again"

        assert chosen_action != -1, "User entered too many illegal actions in UserAgent"
        return chosen_action



def declareWinner(winner, reward):
    """ 
    Prints the winner and reward in a neat format.
    """
    if winner == 1:
        print "Player 1 wins with a reward of", reward
    elif winner == -1:
        print "Player 2 wins with a reward of", reward
    else:
        print "Draw"


