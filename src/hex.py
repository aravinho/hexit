import random

class HexState:


    def __init__(self, dimension, board, reward_type="basic"):
        """
        Creates a new Hex game state.  The board is of size DIMENSION x DIMENSION, and the stones are specified by the given BOARD vector.
        1 represents a white stone, and -1 represents a black.
        Also takes in a string which specifies which reward function to use. 
        "basic" means a simple 1/0/-1 reward. "win_fast" means ((num_total_hexagons + 1) - num_hexagons_played) * winner()
        """

        self._board = board
        self._dimension = dimension
        self._reward_type = reward_type

        # determine whose turn
        sum_pieces = sum(board)
        assert (sum_pieces == 0 or sum_pieces == 1), "Incorrect sum_pieces"
        self._turn = 1 if sum_pieces == 0 else -1

        # determine winner
        self._winner = self.determineWinner()
        self._is_terminal = (self._winner != 0)

        # determine legal actions
        self._legal_actions = set()
        num_total_actions = dimension ** 2
        if not self._is_terminal:
            for pos in range(num_total_actions):
                if board[pos] == 0:
                    self._legal_actions.add(pos)


    def winner(self):
        """
        Returns 0 if the current board has no winner,
        1 if Player 1 has won, and -1 if Player 2 has won.
        """
        return self._winner


    def isTerminalState(self):
        """
        Returns true if the current board is in a terminal state,
        and false otherwise.
        """
        return self._is_terminal


    def reward(self):
        """ 
        Returns 0 if this board is in a nonterminal state,
        a positive float if Player 1 has won, and a negative float if Player 2 has won.
        The internals of the reward function are specified by the _reward_type field.
        (See comment for constructor).
        """

        if not self.isTerminalState():
            return 0.0

        if self._reward_type == "basic":
            return float(self._winner)

        if self._reward_type == "win_fast":
            num_pieces_played = sum([1 if piece != 0 else 0 for piece in self._board])
            return float(self._winner * ((self.numActions() + 1) - num_pieces_played))

        return 0.0


    def turn(self):
        """
        Return 1 if it is Player 1's turn or -1 if Player 2's turn
        Error if the board is in a terminal state.
        """
        return self._turn


    def __eq__(self, other):
        """
        Returns true if this board and the other board are the same.
        Used for testing mostly.
        """
        this_board, other_board = self.board(), other.board()
        if len(this_board) != len(other_board):
            return False 
        for pos in range(len(this_board)):
            if this_board[pos] != other_board[pos]:
                return False

        return True



    def nextState(self, action):
        """
        Returns a new HexState instance for the board obtained by
        taking the given action from this board.
        Errors if the given action is illegal.
        """
        assert self.isLegalAction(action), "Illegal action " + str(action) + " in nextState for Hex"
        if self.isTerminalState():
            return self

        assert self.at(action) == 0, "Cannot make action " + str(action) + " since it is already played (Hex)"
        new_board = list(self.board())
        new_board[action] = self.turn();
        return HexState(self._dimension, new_board, self._reward_type)


    def isLegalAction(self, action):
        """ 
        Returns true if the given action is legal to take, and false otherwise.
        """
        return action in self._legal_actions


    def randomAction(self):
        """
        Return an action, chosen uniformly at random from the available legal actions.
        If there are no legal actions (or if the board is terminal), error.
        """
        num_legal_moves = len(self._legal_actions)
        assert num_legal_moves > 0, "No legal moves available from this hex state."

        r = random.randint(0,num_legal_moves - 1)
        return (list(self._legal_actions))[r]


    def board(self):
        """
        Returns the list that represents the board.
        """
        return self._board


    def numActions(self):
        """
        Returns the number of total actions for this game (dimension x dimension).
        """
        return self._dimension ** 2





    #### Private Helper Functions ####

    def at(self, pos):
        """
        Returns the piece at the given position (either 1, -1 or 0).
        Errors if the given position is out of range.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to at functions"
        return self._board[pos]


    def row(self, pos):
        """
        Returns the row index of a given position.
        Errors if the given position is out of range.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to row function"
        return pos // self._dimension


    def col(self, pos):
        """
        Returns the column index of a given position.
        Errors if the given position is out of range.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to col function"
        return pos % self._dimension


    def pos(self, row, col):
        """
        Returns the index of the position with the given row and column.
        """
        assert 0 <= row and row < self._dimension, "Illegal row argument to pos function"
        assert 0 <= col and col < self._dimension, "Illegal col argument to pos function"
        return (row * self._dimension) + col


    def northwestNeighbor(self, pos):
        """
        Returns the index of the northwest neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to northwestNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if r == 0:
            return -1
        return self.pos(r - 1, c)


    def northeastNeighbor(self, pos):
        """
        Returns the index of the northeast neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to northeastNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if r == 0 or c == self._dimension - 1:
            return -1
        return self.pos(r - 1, c + 1)


    def southwestNeighbor(self, pos):
        """
        Returns the index of the southwest neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to southwestNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if r == self._dimension - 1 or c == 0:
            return -1
        return self.pos(r + 1, c - 1)


    def southeastNeighbor(self, pos):
        """
        Returns the index of the southeast neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to southeastNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if r == self._dimension - 1:
            return -1
        return self.pos(r + 1, c)


    def westNeighbor(self, pos):
        """
        Returns the index of the west neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to westNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if c == 0:
            return -1
        return self.pos(r, c - 1)


    def eastNeighbor(self, pos):
        """
        Returns the index of the east neighbor, or -1 if none exists.
        """
        assert 0 <= pos and pos < self.numActions(), "Illegal position argument to eastNeighbor function"
        r, c = self.row(pos), self.col(pos)
        if c == self._dimension - 1:
            return -1
        return self.pos(r, c + 1)



    def southwardPathExists(self, start_pos, end_row, visited):
        """
        Returns true if there is a southward path connecting Player 1's pieces from the given position
        to some spot in the given end row.
        The visited bitvector is to assist with the path traversal algorithm.
        Errors if the given position or the given end row is out of range.
        """

        assert 0 <= start_pos and start_pos < self.numActions(), "Illegal start position in southwardPathExists function"
        assert 0 <= end_row and end_row < self._dimension, "Illegal end row argument to southwardPathExists function"
        assert len(visited) == self.numActions(), "Visited bitvector is not correct length in southwardPathExists function"

        # this spot must be occupied by Player 1
        if (self.at(start_pos) != 1):
            return False

        # if we are in the end row, return true
        if (self.row(start_pos) == end_row):
            return True

        # mark this spot visisted
        visited[start_pos] = True

        southwest_neighbor = self.southwestNeighbor(start_pos)
        southeast_neighbor = self.southeastNeighbor(start_pos)
        east_neighbor = self.eastNeighbor(start_pos)

        # recurse on southwest neighbor
        if southwest_neighbor != -1 and not visited[southwest_neighbor]:
            if self.southwardPathExists(southwest_neighbor, end_row, visited):
                return True

        # recurse on southeast neighbor
        if southeast_neighbor != -1 and not visited[southeast_neighbor]:
            if self.southwardPathExists(southeast_neighbor, end_row, visited):
                return True

        # recurse on east neighbor
        if east_neighbor != -1 and not visited[east_neighbor]:
            if self.southwardPathExists(east_neighbor, end_row, visited):
                return True

        return False


    def eastwardPathExists(self, start_pos, end_col, visited):
        """
        Returns true if there is an easward path connecting Player 2's pieces from the given start position
        to some spot in the given end col.
        The visited bitvector is to assist with the path traversal algorithm.
        Errors if the given position or the given end col is out of range.
        """

        assert 0 <= start_pos and start_pos < self.numActions(), "Illegal start position in southwardPathExists function"
        assert 0 <= end_col and end_col < self._dimension, "Illegal end col argument to southwardPathExists function"
        assert len(visited) == self.numActions(), "Visited bitvector is not correct length in southwardPathExists function"

        # this spot must be occupied by Player 2
        if (self.at(start_pos) != -1):
            return False

        # if we are in the end row, return true
        if (self.col(start_pos) == end_col):
            return True

        # mark this spot visisted
        visited[start_pos] = True

        east_neighbor = self.eastNeighbor(start_pos)
        northeast_neighbor = self.northeastNeighbor(start_pos)
        southeast_neighbor = self.southeastNeighbor(start_pos)

        # recurse on east neighbor
        if east_neighbor != -1 and not visited[east_neighbor]:
            if self.eastwardPathExists(east_neighbor, end_col, visited):
                return True

        # recurse on northeast neighbor
        if northeast_neighbor != -1 and not visited[northeast_neighbor]:
            if self.eastwardPathExists(northeast_neighbor, end_col, visited):
                return True

        # recurse on southeast neighbor
        if southeast_neighbor != -1 and not visited[southeast_neighbor]:
            if self.eastwardPathExists(southeast_neighbor, end_col, visited):
                return True

        return False


    def northSouthPathExists(self):
        """
        Returns true if there is a path of Player 1's pieces from the north row to the south row.
        """
        end_row = self._dimension - 1

        # look for southward paths starting at all the positions on the top row
        for pos in range(self._dimension):
            visited = [False for a in range(self.numActions())]
            if self.southwardPathExists(pos, end_row, visited):
                return True

        return False


    def westEastPathExists(self):
        """
        Returns true if there is a path of Player 2's pieces from the west col to the east col.
        """
        end_col = self._dimension - 1

        # look for eastward paths starting at all positions in the left col
        for pos in range(0, self.numActions(), self._dimension):
            visited = [False for a in range(self.numActions())]
            if self.eastwardPathExists(pos, end_col, visited):
                return True

        return False


    def determineWinner(self):
        """
        Returns 1 if Player 1 wins this board, -1 if Player 2 wins, and 0 if nobody wins.
        """

        # check if there is a North to South path
        if self.northSouthPathExists():
            return 1

        # check if there is a West to East path
        if self.westEastPathExists():
            return -1

        return 0



    #### Utility Functions ####


    def printRow(self, row):
        """
        Helper function to print one row in a human readable format.
        """
        # print offset
        for off in range(row):
            print " ",

        # print pieces
        for col in range(self._dimension):
            print pieceAsString(self.at(self.pos(row, col))) + "  ",

        print

    def printBoard(self):
        """
        Returns a human readable representaion of the board.
        """
        for row in range(self._dimension):
            self.printRow(row)


    def asCSVString(self):
        """
        Returns the board vector, represented as a CSV string.
        """
        s = ""
        num_actions = self.numActions()
        for pos, piece in enumerate(self.board()):
            s += str(piece)
            if (pos != num_actions - 1):
                s += ","
        return s



#### Random Utility Helper Functions #####

def pieceAsString(piece):
    """ 
    Returns a human-readable string representing the given piece.
    """
    if piece == 1:
        return "X"
    if piece == -1:
        return "O"
    return "_"


