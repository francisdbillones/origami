# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

TARGET = run

SRCS = arithmetic_coding.cpp ByteStaticModel.cpp

OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)