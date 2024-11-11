.PHONEY: build, run

build: program

run: program
	./program $(file)

program: main.cpp uselibpng.c
	clang++ -std=c++11 -O3 -I/opt/homebrew/include -L/opt/homebrew/lib main.cpp uselibpng.c -lpng -o program