import numpy as np
import os
import sys
import random

from nn import ExitNN

NUM_ACTIONS = 9

def processRow(squares, legal_actions, winning_actions):

    perm = 0
    piece = 0
    turn = 1

    reward = 0

    for i in range(3):
        perm = 0

        for j in range(3):
            piece = squares[3*i+j]
            perm += (3 ** j) * piece
            if piece == 0:
                legal_actions.add(3*i + j)
            else:
                turn *= -1

        if perm == 13:
            reward = 1
            break
        if perm == -13:
            reward = -1
            break

        if perm == 12 * turn:
            winning_actions.add(3*i + 2)
        if perm == 10 * turn:
            winning_actions.add(3*i + 1)
        if perm == 4 * turn:
            winning_actions.add(3*i + 0)

    return (reward, legal_actions, winning_actions)

def processCol(squares, legal_actions, winning_actions):

    perm = 0
    piece = 0
    turn = -1

    reward = 0

    for j in range(3):
        perm = 0

        for i in range(3):
            piece = squares[3*i + j]
            perm += (3 ** i) * piece
            if piece == 0:
                legal_actions.add(3*i + j)
            else:
                turn *= -1

        if perm == 13:
            reward = 1
            break
        if perm == -13:
            reward = -1
            break

        if perm == 12 * turn:
            winning_actions.add(3*2 + j)
        if perm == 10 * turn:
            winning_actions.add(3*1 + j)
        if perm == 4 * turn:
            winning_actions.add(3*0 + j)

    return (reward, legal_actions, winning_actions) 


def processMajorDiag(squares, legal_actions, winning_actions):
    diag = [0,4,8]

    perm, piece, turn = 0, 0, 1
    reward = 0

    for i in range(3):
        piece = squares[diag[i]]
        perm += (3 ** i) * piece
        if piece == 0:
            legal_actions.add(diag[i])
        else:
            turn *= -1

        if perm == 13:
            reward = 1
            break
        if perm == -13:
            reward = -1
            break

        if perm == 12 * turn:
            winning_actions.add(diag[0])
        if perm == 10 * turn:
            winning_actions.add(diag[1])
        if perm == 4 * turn:
            winning_actions.add(diag[2])

    return (reward, legal_actions, winning_actions)


def processMinorDiag(squares, legal_actions, winning_actions):
    diag = [2,4,6]

    perm, piece, turn = 0, 0, 1
    reward = 0

    for i in range(3):
        piece = squares[diag[i]]
        perm += (3 ** i) * piece
        if piece == 0:
            legal_actions.add(diag[i])
        else:
            turn *= -1

        if perm == 13:
            reward = 1
            break
        if perm == -13:
            reward = -1
            break

        if perm == 12 * turn:
            winning_actions.add(diag[0])
        if perm == 10 * turn:
            winning_actions.add(diag[1])
        if perm == 4 * turn:
            winning_actions.add(diag[2])

    return (reward, legal_actions, winning_actions)


class Tictactoe:

    def __init__(self, squares):
        turn = 0
        reward = 0
        legal_actions = set()
        winning_actions = set()

        sum_squares = sum(squares)
        if sum_squares == 0:
            turn = 1
        elif sum_squares == 1:
            turn = -1
        else:
            raise ValueError('Sum of squares is not -1 or 1')


        reward, legal_actions, winning_actions = processRow(squares, legal_actions, winning_actions)
        if reward == 0:
            reward, legal_actions, winning_actions = processCol(squares, legal_actions, winning_actions)
        if reward == 0:
            reward, legal_actions, winning_actions = processMajorDiag(squares, legal_actions, winning_actions)
        if reward == 0:
            reward, legal_actions, winning_actions = processMinorDiag(squares, legal_actions, winning_actions)

        self._sq = squares
        self._turn = turn
        self._is_terminal = (reward != 0)
        self._reward = reward
        self._legal_actions = legal_actions
        self._winning_actions = winning_actions

    def winner(self):
        return self._reward

    def isTerminalState(self):
        if (self._reward != 0):
            return True

        num_squares_played = 0
        for sq in self._sq:
            if (sq != 0):
                num_squares_played += 1

        return num_squares_played == 9

    def reward(self):
        num_squares_played = 0
        for sq in self._sq:
            if (sq != 0):
                num_squares_played += 1

        return self._reward * ((NUM_ACTIONS + 1) - num_squares_played)

    def squares(self):
        return self._sq

    def __eq__(self, other):
        return self.squares() == other.squares()

    def nextState(self, action):
        if self.isTerminalState():
            return self

        if (self._sq[action] != 0):
            raise ValueError("Illegal action in Tictactoe.nextState: " + str(action))

        new_sq = list(self._sq)
        new_sq[action] = self._turn
        return Tictactoe(new_sq)

    def isLegalAction(self, action):
        return action in self._legal_actions

    def randomAction(self):
        num_legal_moves = len(self._legal_actions)
        if num_legal_moves == 0:
            raiseValueError("No legal moves available in Tictactoe.randomAction")

        r = random.randint(0,num_legal_moves - 1)
        return (list(self._legal_actions))[r]

    def turn(self):
        return self._turn

    def numActions():
        return NUM_ACTIONS

    def pieceAsString(self, piece):
        if piece == 1:
            return 'X'
        if piece == -1:
            return 'O'
        return ' '

    def asNumpyArray(self):
        return np.array(self._sq).astype(np.float32)

    def printBoard(self):
        print "+--+---+---+"
        print self.pieceAsString(self._sq[0]), " |", self.pieceAsString(self._sq[1]), "|", self.pieceAsString(self._sq[2]), "|"
        print "+--+---+---+"
        print self.pieceAsString(self._sq[3]), " |", self.pieceAsString(self._sq[4]), "|", self.pieceAsString(self._sq[5]), "|"
        print "+--+---+---+"
        print self.pieceAsString(self._sq[6]), " |", self.pieceAsString(self._sq[7]), "|", self.pieceAsString(self._sq[8]), "|"
        print "+--+---+---+"

    def asCSVString(self):
        s = ""
        num_sq = NUM_ACTIONS
        assert len(self._sq) == NUM_ACTIONS
        for i, sq in enumerate(self._sq):
            s += str(sq)
            if (i != NUM_ACTIONS - 1):
                s += ","
        return s




t = Tictactoe([1,0,1,0,-1,0,0,-1,1])
assert t.turn() == -1
assert not t.isTerminalState()
assert t.reward() == 0
assert t._legal_actions == {1,3,5,6}
assert t.isLegalAction(3)
assert not t.isLegalAction(7)
assert t.randomAction() in {1,3,5,6}

t = t.nextState(6)
assert t.turn() == 1
assert not t.isTerminalState()
assert t.reward() == 0
assert t._legal_actions == {1,3,5}
assert t.isLegalAction(1)
assert not t.isLegalAction(6)
assert t.randomAction() in {1,3,5}

t = t.nextState(5)
assert t.turn() == -1
assert t.isTerminalState()
assert t.reward() == 3




t = Tictactoe([-1,0,1,0,-1,1,1,0,-1])
assert t.turn() ==1
assert t.isTerminalState()
assert t.reward() == -4

t = Tictactoe([0,1,-1,-1,1,0,0,1,0])
assert t.turn() == -1
assert t.isTerminalState()
assert t.reward() == 5


t = Tictactoe([0,0,-1,0,-1,0,1,1,1])
assert t.turn() == -1
assert t.isTerminalState()
assert t.reward() == 5





class GameAgent:

    def __init__(self, game):
        self.game = game

    def getAction(self, board):
        return -1

class NNAgent(GameAgent):

    def __init__(self, game, model_dir, sample=True):
        self.game = game
        self.model_dir = model_dir
        self.sample = sample
        self.nn = ExitNN() # Eventually customize args to NN based on whic game

    def getAction(self, board):
        # create np array, feed to predict method
        state = board.asNumpyArray()
        
        legal_action = False
        t = 0
        while not legal_action and t < 10:
            action = self.nn.predictSingle(state, self.model_dir, self.sample)
            if board.isLegalAction(action):
                legal_action = True
            t += 1

        return action

class UserAgent(GameAgent):

    def __init__(self, game):
        self.game = game

    def getAction(self, board):
        legal_action = False
        while not legal_action:
            action = input("Enter a legal action number: ")
            legal_action = board.isLegalAction(action)
            if not legal_action:
                print "Action", action, "is illegal"

        return action

def declareWinner(winner, reward):
    if winner == 1:
        print "Player 1 (X) wins with a reward of", reward
    elif winner == -1:
        print "Player 2 (O) wins with a reward of", reward
    else:
        print "Draw"

# Returns a list of all the non-terminal game states in this game (episode)
def playGame(game, p1_agent, p2_agent, random_first_move=True, display_board=False):
    
    if game == "tictactoe":
        board = Tictactoe([0,0,0,0,0,0,0,0,0])

    game_states = []

    if random_first_move:
        game_states.append(board)
        random_action = board.randomAction()
        board = board.nextState(random_action)


    while not board.isTerminalState():
        if display_board:
            print
            board.printBoard()
            print

        game_states.append(board)

        if board.turn() == 1:
            action = p1_agent.getAction(board)

        if board.turn() == -1:
            action = p2_agent.getAction(board)


        board = board.nextState(action)

    if display_board:
        board.printBoard()
        print

    reward = board.reward()
    winner = board.winner()
    if display_board:
        declareWinner(winner, reward)

    return game_states


def generateDataBatch(game, num_episodes, p1_agent, p2_agent, log_every=2**6, random_first_move=True, display_board=False):
    states = [None for episode_num in range(num_episodes)]

    for episode_num in range(num_episodes):
        game_states = playGame(game, p1_agent, p2_agent, random_first_move, display_board)
        random_state = random.choice(game_states)
        states[episode_num] = random_state

    return states

def writeBatchToFile(batch, data_file):
    f = open(data_file, 'w')
    for b in batch:
        f.write(b.asCSVString())
        f.write("\n")

    f.close()

def main():
    model_dir = "models/tictactoe/test_batch/"
    game = "tictactoe"
    sample=True
    num_episodes = 4

    if len(sys.argv) != 2:
        print "Wrong number of arguments"
        exit(1)

    option = sys.argv[1]

    if option == "--train":
        print_every = 10
        save_every = 10000
        num_epochs = 500
        from_scratch = True
        nn = ExitNN(print_every=print_every, save_every=save_every)
        data_dir = "data/in/train/tictactoe/raw_mcts/"
        model_ckpt_dir = "models/tictactoe/basic_model/"
        nn.train(data_dir, model_ckpt_dir, from_scratch=from_scratch, num_epochs=num_epochs)
        print "DONE TRAINING"

    if option == "--play":
        model_ckpt_dir = "models/tictactoe/basic_model/"
        sample = True
        num_episodes = 2 ** 14
        p1_agent = NNAgent(game, model_dir, sample)
        p2_agent = NNAgent(game, model_dir, sample)
        display_board = False
        random_first_move = True
        log_every = 2**7
        data_batch = generateDataBatch(game, num_episodes, p1_agent, p2_agent, log_every, random_first_move, display_board)
        data_outfile = "data/in/train/tictactoe/nn_states"
        writeBatchToFile(data_batch, data_outfile)
        print "DONE GENERATING GAME STATES"
        
    print 

   


    

if __name__ == '__main__':
    main()





    

 