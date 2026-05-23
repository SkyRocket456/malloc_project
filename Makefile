# =============================================================================
# Makefile  —  my_malloc project
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g -O0
TARGET   := run_tests
SRCS     := my_malloc.cpp tests.cpp
OBJS     := $(SRCS:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp my_malloc.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
