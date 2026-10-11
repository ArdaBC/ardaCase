CXX ?= g++
CXXFLAGS ?= -std=c++14 -O2 -Wall

SOURCES = main.cpp Util.cpp ItchDecoder.cpp MoldUdp64.cpp OrderBook.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(wildcard *.hpp)

main: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f main $(OBJECTS)
