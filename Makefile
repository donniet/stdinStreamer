CFLAGS= -Wall
CPPFLAGS= -std=c++11 -lboost_program_options -lpthread

main: main.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -o $@ $^
