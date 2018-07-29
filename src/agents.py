from exit_nn import *
from hex import HexState

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

    def __init__(self, game, model_spec, model_path, sample=True):
        """
        The "game" is a string that specifies what game this agent is playing.
        The "model_spec" argument is the name of the spec file for the model (see comments for ExitNN constructors)
        The model_path argument is a string for the filepath at which the saved NN model sits.
        The sample argument tells the NN whether to sample the best action.
        If false, it just chooses the action with the highest score.
        """
        self.game = game
        self.model_spec = model_spec
        self.model_path = model_path
        self.sample = sample
        self.nn = ExitNN(model_spec=model_spec) # Eventually customize args to NN based on which game


    def getAction(self, state):
        """
        Returns a legal action for the given state. The action is chosen by the NN.
        If the NN keeps repeatedly suggesting illegal actions, eventually error.
        """

        # create np array, feed to predict method
        state = state.asNumpyArray()
        
        chosen_action = -1
        legal_action = False
        t = 0
        while not legal_action and t < 10:
            chosen_action = self.nn.predictSingle(state, self.model_path, self.sample)
            if state.isLegalAction(chosen_action):
                legal_action = True
            t += 1

        assert chosen_action != -1, "No legal action could be found by NNAgent"

        return action


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


