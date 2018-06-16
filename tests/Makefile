TEST_OBJS	= test_tictactoe.o #test_Mcts.o
SOURCE_HEADERS	= tictactoe.h

CC 	= gcc
#CFLAGS	= 

test: test.o $(TEST_OBJS)
	$(CC) -o test test.o

test.o: test.cpp $(SOURCE_HEADERS)
	$(CC) test.cpp

test: main.o test_tictactoe.o tictactoe.o

main.o: main.cc test_tictactoe.h $(SOURCE_HEADERS)
	gcc -c main.cc

test_tictactoe.o: test_tictactoe.cc test_tictactoe.h ../src/tictactoe.h
	$(CC) -c test_tictactoe.cc
