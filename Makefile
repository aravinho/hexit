SRC_OBJ_FILES	= obj/tictactoe.o obj/mcts.o obj/mcts-thread-manager.o obj/hex-state.o obj/play-hex.o
TEST_EXE_FILE 	= hexit-tests
TEST_OBJ_FILES	= obj/hexit-tests.o obj/test-tictactoe.o obj/test-thread-manager.o obj/test-utils.o obj/test-mcts.o
PLAY_EXE_FILE 	= play-hex

SRC_HEADER_FILES = src/tictactoe.h src/mcts.h src/hex_state.h src/thread_manager.h src/play_hex.h
SRC_CC_FILES = src/tictactoe.cc src/mcts.cc src/hex_state.cc src/thread_manager.cc src/play_hex.cc
TEST_HEADER_FILES = tests/test_tictactoe.h tests/test_utils.h tests/test_thread_manager.h tests/test_mcts.h
TEST_CC_FILES = tests/

CC	= g++ -std=c++11
INC_FLAGS = -I src -I tests

play: $(PLAY_EXE_FILE)
	./$(PLAY_EXE_FILE)

check : $(TEST_EXE_FILE)
	./$(TEST_EXE_FILE)

$(PLAY_EXE_FILE): $(SRC_OBJ_FILES)
	$(CC) -o $(PLAY_EXE_FILE) $(SRC_OBJ_FILES)

$(TEST_EXE_FILE): $(SRC_OBJ_FILES) $(TEST_OBJ_FILES)
	$(CC) -o $(TEST_EXE_FILE) $(TEST_OBJ_FILES) $(SRC_OBJ_FILES)

# Test Objects
obj/hexit-tests.o: tests/hexit_tests.cc tests/test_tictactoe.h
	$(CC) -c -o obj/hexit-tests.o $(INC_FLAGS) tests/hexit_tests.cc

obj/test-tictactoe.o: tests/test_tictactoe.cc tests/test_tictactoe.h src/tictactoe.h tests/test_utils.h
	$(CC) -c -o obj/test-tictactoe.o $(INC_FLAGS) tests/test_tictactoe.cc

obj/test-thread-manager.o: tests/test_thread_manager.cc tests/test_thread_manager.h src/mcts_thread_manager.h tests/test_utils.h
	$(CC) -c -o obj/test-thread-manager.o $(INC_FLAGS) tests/test_thread_manager.cc 

obj/test-utils.o: tests/test_utils.cc tests/test_utils.h
	$(CC) -c -o obj/test-utils.o $(INC_FLAGS) tests/test_utils.cc

obj/test-mcts.o: tests/test_mcts.cc tests/test_mcts.h
	$(CC) -c -o obj/test-mcts.o $(INC_FLAGS) tests/test_mcts.cc

# Source Objects
obj/tictactoe.o: src/tictactoe.cc src/tictactoe.h
	$(CC) -c -o obj/tictactoe.o $(INC_FLAGS) src/tictactoe.cc


obj/mcts.o: src/mcts.cc src/mcts.h src/mcts_thread_manager.cc src/mcts_thread_manager.h
	$(CC) -c -o obj/mcts.o $(INC_FLAGS) src/mcts.cc

obj/hex-state.o: src/hex_state.cc src/hex_state.h
	$(CC) -c -o obj/hex-state.o $(INC_FLAGS) src/hex_state.cc

obj/mcts-thread-manager.o: src/mcts_thread_manager.cc src/mcts_thread_manager.h src/mcts.h
	$(CC) -c -o obj/mcts-thread-manager.o $(INC_FLAGS) src/mcts_thread_manager.cc

obj/play-hex.o: src/play_hex.cc src/play_hex.h src/mcts.h
	$(CC) -c -o obj/play-hex.o $(INC_FLAGS) src/play_hex.cc
