CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -Werror -O2
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: acaderp

acaderp: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: acaderp
	./run_tests.sh
	./run_unit_checks.sh

clean:
	rm -f $(OBJECTS) acaderp

.PHONY: all test clean
