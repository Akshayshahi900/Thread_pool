CXX = g++

CXXFLAGS = -g -Wall -Wextra -Wpedantic

TARGET = server

SRC = src/threadPool.cpp src/main.cpp
all:
	$(CXX)	$(CXXFLAGS)	$(SRC)	-Iinclude	-o	$(TARGET)
