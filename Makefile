CXX = g++

all:
	$(CXX) main.cc -o level_editor -Wall -g -std=c++0x -lncurses
	./level_editor

