# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -O3 -std=c++17

all: compress decompress profile

compress: compress.cpp arithmetic_coding.cpp byte_static_model.cpp
	$(CXX) $(CXXFLAGS) compress.cpp -o compress

decompress: decompress.cpp arithmetic_coding.cpp byte_static_model.cpp
	$(CXX) $(CXXFLAGS) decompress.cpp -o decompress

profile: profile.cpp arithmetic_coding.cpp byte_static_model.cpp
	$(CXX) $(CXXFLAGS) profile.cpp -o profile

clean:
	rm -f compress decompress profile *.o