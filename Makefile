SRC_OBJ_FILES	= obj/tictactoe.o obj/mcts.o obj/mcts-thread-manager.o
TEST_EXE_FILE 	= hexit-tests
TEST_OBJ_FILES	= obj/hexit-tests.o obj/test-tictactoe.o obj/test-thread-manager.o obj/test-utils.o

CC	= g++ -std=c++11
INC_FLAGS = -I src -I tests

check : $(TEST_EXE_FILE)
	./$(TEST_EXE_FILE); diff data/tmp/nodes_0 data/exp_nodes_0; diff data/tmp/nodes_1 data/exp_nodes_1; diff data/tmp/nodes_2 data/exp_nodes_2; diff data/tmp/nodes_3 data/exp_nodes_3;

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

# Source Objects
obj/tictactoe.o: src/tictactoe.cc src/tictactoe.h
	$(CC) -c -o obj/tictactoe.o $(INC_FLAGS) src/tictactoe.cc


obj/mcts.o: src/mcts.cc src/mcts.h
	$(CC) -c -o obj/mcts.o $(INC_FLAGS) src/mcts.cc

obj/mcts-thread-manager.o: src/mcts_thread_manager.cc src/mcts_thread_manager.h src/mcts.h
	$(CC) -c -o obj/mcts-thread-manager.o $(INC_FLAGS) src/mcts_thread_manager.cc
