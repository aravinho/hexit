SRC_OBJ_FILES	= obj/tictactoe.o
TEST_EXE_FILE 	= hexit-tests
TEST_OBJ_FILES	= obj/hexit-tests.o obj/test-tictactoe.o obj/tictactoe.o

CC	= g++ -std=c++11
INC_FLAGS = -I src -I tests

check : $(TEST_EXE_FILE)
	./$(TEST_EXE_FILE)

$(TEST_EXE_FILE): $(TEST_OBJ_FILES)
	$(CC) -o $(TEST_EXE_FILE) $(TEST_OBJ_FILES)


obj/hexit-tests.o: tests/hexit_tests.cc tests/test_tictactoe.h
	$(CC) -c -o obj/hexit-tests.o $(INC_FLAGS) tests/hexit_tests.cc

obj/tictactoe.o: src/tictactoe.cc src/tictactoe.h
	$(CC) -c -o obj/tictactoe.o $(INC_FLAGS) src/tictactoe.cc

obj/test-tictactoe.o: tests/test_tictactoe.cc tests/test_tictactoe.h src/tictactoe.h
	$(CC) -c -o obj/test-tictactoe.o $(INC_FLAGS) tests/test_tictactoe.cc
