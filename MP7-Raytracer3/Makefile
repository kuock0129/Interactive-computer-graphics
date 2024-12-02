.PHONY: build run clean

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++14 -O3 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lpng

# Source files
SRCS = main.cpp Math.cpp uselibpng.c

build: program

run: program
	./program $(file)

program: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o program

clean:
	rm -rf program *.png *.o