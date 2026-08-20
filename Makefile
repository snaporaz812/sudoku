all: sudoku

toyforth: sudoku.c
	$(CC) sudoku.c -Wall -W -O2 -std=c2x -o sudoku

clean:
	rm -rf sudoku