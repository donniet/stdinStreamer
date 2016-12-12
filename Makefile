CFLAGS= -Wall
CPPFLAGS= -std=c++11

main: main.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -o $@ $^
