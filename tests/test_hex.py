import sys
sys.path.append('/Users/aravinho/programs/hexit/src')
from hex import HexState


test_dimension = 3
boards = {
	"empty": HexState(test_dimension, [0,0,0,0,0,0,0,0,0], "basic"),
	"simple_draw": HexState(test_dimension, [1,0,0,0,-1,0,0,0,1], "win_fast"),
	"complex_draw": HexState(test_dimension, [1,1,-1,1,-1,-1,0,1,-1], "basic"),

	"white_move_4": HexState(test_dimension, [0,0,0,0,1,0,0,0,0], "basic"),
	"black_move_3": HexState(test_dimension, [1,0,0,-1,-1,0,0,0,1], "win_fast"),
	"white_move_6": HexState(test_dimension, [1,1,-1,1,-1,-1,1,1,-1], "basic"),

	"simple_p1_win": HexState(test_dimension, [0,0,1,-1,1,0,0,1,-1], "win_fast"),
	"complex_p1_win": HexState(test_dimension, [1,1,-1,-1,1,1,-1,-1,1], "basic"),
	"simple_p2_win": HexState(test_dimension, [0,0,1,0,1,-1,-1,-1,1], "win_fast"),
	"complex_p2_win": HexState(test_dimension, [-1,1,-1,-1,-1,1,1,1,1], "basic")
}


def testWinnerHex():
	
	# test empty
	assert boards["empty"].winner() == 0, "winner should be 0"

	# test basic early game
	assert boards["simple_draw"].winner() == 0, "winner should be 0"

	# test complex non-win
	assert boards["complex_draw"].winner() == 0, "winner should be 0"

	# test basic Player 1 win
	assert boards["simple_p1_win"].winner() == 1, "winner should be 1"

	# test complex Player 1 win
	assert boards["complex_p1_win"].winner() == 1, "Winner should be 1"

	# test basic Player 2 win
	assert boards["simple_p2_win"].winner() == -1, "Winner should be -1"

	# test complex Player 2 win
	assert boards["complex_p2_win"].winner() == -1, "Winner should be -1"


def testIsTerminalStateHex():
	
	# test empty
	assert not boards["empty"].isTerminalState(), "should be nonterminal"

	# test basic early game
	assert not boards["simple_draw"].isTerminalState(), "should be nonterminal"

	# test complex non-win
	assert not boards["complex_draw"].isTerminalState(), "should be nonterminal"

	# test basic Player 1 win
	assert boards["simple_p1_win"].isTerminalState(), "should be terminal"

	# test complex Player 1 win
	assert boards["complex_p1_win"].isTerminalState(), "should be terminal"

	# test basic Player 2 win
	assert boards["simple_p2_win"].isTerminalState(), "should be terminal"

	# test complex Player 2 win
	assert boards["complex_p2_win"].isTerminalState(), "should be terminal"


def testRewardHex():
	
	# test empty
	assert boards["empty"].reward() == 0, "Reward should be 0"

	# test basic early game
	assert boards["simple_draw"].reward() == 0, "Reward should be 0"

	# test complex non-win
	assert boards["complex_draw"].reward() == 0, "Reward should be 0"

	# test basic Player 1 win
	assert boards["simple_p1_win"].reward() == 5, "Reward should be 5"

	# test complex Player 1 win
	assert boards["complex_p1_win"].reward() == 1, "Reward should be 1"

	# test basic Player 2 win
	assert boards["simple_p2_win"].reward() == -4, "Reward should be -4"

	# test complex Player 2 win
	assert boards["complex_p2_win"].reward() == -1, "Reward should be -1"


def testTurnHex():
	
	# test empty
	assert boards["empty"].turn() == 1, "Turn should be 1"

	# test basic early game
	assert boards["simple_draw"].turn() == -1, "Turn should be -1"

	# test complex non-win
	assert boards["complex_draw"].turn() == 1, "Turn should be 1"


def testNextStateHex():

	# test that next state produces the correct board
	assert boards["empty"].nextState(4) == boards["white_move_4"], "White move 4"
	assert boards["simple_draw"].nextState(3) == boards["black_move_3"], "Black move 3"
	assert boards["complex_draw"].nextState(6) == boards["white_move_6"], "White move 6"


def testIsLegalActionHex():
	
	# all moves legal from start, none legal from terminal state
	assert boards["empty"].isLegalAction(5), "Any move legal at first"
	assert boards["empty"].isLegalAction(8), "Any move legal at first"
	assert not boards["simple_p1_win"].isLegalAction(0), "No move legal from terminal state"
	assert not boards["complex_p2_win"].isLegalAction(7), "No move legal from terminal state"

	assert boards["simple_draw"].isLegalAction(2), "Legal action 2"
	assert not boards["simple_draw"].isLegalAction(4), "Illegal action 4"
	assert boards["complex_draw"].isLegalAction(6), "Legal action 6"
	assert not boards["complex_draw"].isLegalAction(3), "Illegal action 3"


def testRandomActionHex():
	
	a_set = {0,1,2,3,4,5,6,7,8}
	b_set = {1,2,3,5,6,7}
	c_set = {6}

	# try several random action samples and make sure they are in the correct range
	for t in range(10):

		a = boards["empty"].randomAction()
		b = boards["simple_draw"].randomAction()
		c = boards["complex_draw"].randomAction()

		assert a in a_set, "Random move for empty"
		assert b in b_set, "Random move for simple draw"
		assert c in c_set, "Random move for complex draw"
	


def testAsCSVStringHex():
	
	assert boards["empty"].asCSVString() == "0,0,0,0,0,0,0,0,0", "Empty board CSV String incorrect"
	assert boards["complex_p2_win"].asCSVString() == "-1,1,-1,-1,-1,1,1,1,1", "CSV String incorrect for complex p2 win"



def testPrintBoardHex():

	print "Printing sample board [0,0,1,0,1,-1,-1,-1,-1]"
	boards["simple_p2_win"].printBoard()

def runHexTests():
	print "Running Hex Tests...\n"
	testWinnerHex()
	testIsTerminalStateHex()
	testRewardHex()
	testTurnHex()
	testNextStateHex()
	testIsLegalActionHex()
	testRandomActionHex()
	testAsCSVStringHex()
	testPrintBoardHex()
	print "Done running Hex Tests\n"

def main():
	print "hi"

if __name__ == '__main__':
	main()