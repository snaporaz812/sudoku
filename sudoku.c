#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>  


// ============================= DATA STRUCTURES =============================

typedef struct Cell {
    int i;          // cell's value
    bool isfixed;   // if true, i cannot be modified
} Cell;

typedef struct SudokuBoard { 
    Cell *cells[81]; // Whole 9x9 grid
} SudokuBoard;


// ============== ALLOCATION WRAPPERS ==============

// Try to allocate n bytes, else close the program.
void *xmalloc(size_t size) {
	void *ptr = malloc(size);
	if (ptr == NULL) {
		fprintf(stderr, "Out of memory allocating %zu bytes\n", size);
		exit(1);
	}
	return ptr;
}


// =========================== BOARD OBJECTS ============================
/* Allocate and initialize objects. */

Cell *createCell(int i, bool isfixed) {
    Cell *c = xmalloc(sizeof(*c));
    c->i = i;
    c->isfixed = isfixed;
    return c;
};

SudokuBoard *createSudokuBoard(void) {
    SudokuBoard *sb = xmalloc(sizeof(*sb));

    // Initialize cells
    for (int i=0; i <=9*9; i++) {    
        sb->cells[i] = createCell(i, false);
    }
    return sb;
};

// ------ Probably useless funtion ------
/* Set the cell's 'isfixed' variable either to true or false. */
void setCellState(Cell *c, bool state) {
    c->isfixed = state;
};
// ---------------------------------------

/* Set the cell's 'i' variable  to the desired value,
 * provided the cell is not fixed. */
void setCellValue(Cell *c, int i) {
    if (!c->isfixed) c->i = i;
}

void printBoard(SudokuBoard *sb) { // FIXME
    for (int i=0; i<=81; i++) {
        if (9*i % 3 == 0) printf(" _________________\n");
        if (i % 3 == 0)printf("|");
        printf("%d", sb->cells[i]->i);
        if (i % 9 == 0) printf("\n");
    }

};


// ============================= MAIN =============================

int main(int argc, char **argv) {

    // ============== INITIALIZE THE SUDOKU BOARD ============
    SudokuBoard *sb = createSudokuBoard();


    // ============== SHOW THE GAME ON SCREEN ============
    printBoard(sb);


    // ============== SIVALLETTO SEQUENCE ============
    // Destroy the sudoku board
    free(sb);

    return 0;
}

    /*  _________________
     * |# # #|# # #|# # #| 
     * |# # #|# # #|# # #|
     * |# # #|# # #|# # #|
     *  _________________
     * |# # #|# # #|# # #|
     * |# # #|# # #|# # #|
     * |# # #|# # #|# # #|
     *  _________________
     * |# # #|# # #|# # #|
     * |# # #|# # #|# # #|
     * |# # #|# # #|# # #|
     * 
     * * * * * * * * * * */