SRC_OBJ_FILES	= obj/tictactoe.o obj/mcts.o obj/mcts-thread-manager.o obj/hex-state.o obj/play-hex.o obj/gui.o obj/main.o
TEST_EXE_FILE 	= bin/hexit-tests
TEST_OBJ_FILES	= obj/hexit-tests.o obj/test-tictactoe.o obj/test-thread-manager.o obj/test-utils.o obj/test-mcts.o obj/test-hex.o
PLAY_EXE_FILE 	= bin/play-hex
MAIN_OBJ_FILES	= obj/tictactoe.o obj/mcts.o obj/main.o obj/mcts-thread-manager.o
MAIN_EXE_FILE	= bin/main-exe
NEW_TEST_EXE_FILE 	= bin/new-hexit-tests
NEW_TEST_OBJ_FILES	= obj/test-hex.o obj/test-utils.o obj/hexit-tests.o
NEW_TEST_SRC_OBJ_FILES = obj/hex-state.o

SRC_HEADER_FILES = src/tictactoe.h src/mcts.h src/hex_state.h src/thread_manager.h src/play_hex.h src/gui.h src/main.h
SRC_CC_FILES = src/tictactoe.cc src/mcts.cc src/hex_state.cc src/thread_manager.cc src/play_hex.cc src/gui.cc src/main.cc src/env_state.cc
TEST_HEADER_FILES = tests/test_tictactoe.h tests/test_utils.h tests/test_thread_manager.h tests/test_mcts.h tests/test_hex.h
TEST_CC_FILES = tests/

CC_OPTIONS = -fno-rtti -fno-exceptions -fstrict-aliasing -Wall -D__WXMSW__ -D__GNUWIN32__ -D__WIN95__ #-fno-pcc-struct-return -fvtable-thunks
LINKER_OPTIONS = -lwxmsw -lole32 -lwsock32 -lcomctl32 -lctl3d32 -lgcc -lstdc++ -lshell32 -loleaut32 -ladvapi32 -luuid -lodbc32
#(https://wiki.wxwidgets.org/Writing_Your_First_Application-Introduction)

CC	= g++ -std=c++11
INC_FLAGS = -I src -I tests

play: $(PLAY_EXE_FILE)
	./$(PLAY_EXE_FILE)

check : $(TEST_EXE_FILE)
	./$(TEST_EXE_FILE)

main: $(MAIN_EXE_FILE)
	# ./$(MAIN_EXE_FILE)

test: $(NEW_TEST_EXE_FILE)
	./$(NEW_TEST_EXE_FILE)

$(MAIN_EXE_FILE): $(MAIN_OBJ_FILES)
	$(CC) -o $(MAIN_EXE_FILE) $(MAIN_OBJ_FILES)

$(PLAY_EXE_FILE): $(SRC_OBJ_FILES)
	$(CC) -o $(LINKER_OPTIONS) $(PLAY_EXE_FILE) $(SRC_OBJ_FILES)

$(TEST_EXE_FILE): $(SRC_OBJ_FILES) $(TEST_OBJ_FILES)
	$(CC) -o $(TEST_EXE_FILE) $(TEST_OBJ_FILES) $(SRC_OBJ_FILES)

$(NEW_TEST_EXE_FILE): $(NEW_TEST_SRC_OBJ_FILES) $(NEW_TEST_OBJ_FILES)
	$(CC) -o $(NEW_TEST_EXE_FILE) $(NEW_TEST_OBJ_FILES) $(NEW_TEST_SRC_OBJ_FILES)
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

obj/test-hex.o: tests/test_hex.cc tests/test_hex.h
	$(CC) -c -o obj/test-hex.o $(INC_FLAGS) tests/test_hex.cc

# Source Objects
obj/tictactoe.o: src/tictactoe.cc src/tictactoe.h
	$(CC) -c -o obj/tictactoe.o $(INC_FLAGS) src/tictactoe.cc


obj/mcts.o: src/mcts.cc src/mcts.h src/mcts_thread_manager.cc src/mcts_thread_manager.h
	$(CC) -c -o obj/mcts.o $(INC_FLAGS) src/mcts.cc

obj/hex-state.o: src/hex_state.cc src/hex_state.h src/env_state.h
	$(CC) -c -o obj/hex-state.o $(INC_FLAGS) src/hex_state.cc

obj/mcts-thread-manager.o: src/mcts_thread_manager.cc src/mcts_thread_manager.h src/mcts.h
	$(CC) -c -o obj/mcts-thread-manager.o $(INC_FLAGS) src/mcts_thread_manager.cc

obj/play-hex.o: src/play_hex.cc src/play_hex.h src/mcts.h
	$(CC) -c -o obj/play-hex.o $(INC_FLAGS) src/play_hex.cc

obj/gui.o: src/gui.cc src/gui.h
	$(CC) -c -o obj/gui.o $(INC_FLAGS) src/gui.cc

obj/main.o: src/main.cc src/main.h
	$(CC) -c -o obj/main.o $(INC_FLAGS) src/main.cc




