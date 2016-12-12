CFLAGS= -Wall
CPPFLAGS= -std=c++11 -lboost_program_options

main: main.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -o $@ $^
