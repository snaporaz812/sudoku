#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>  
#include <stdarg.h>
#include <string.h>
#include <time.h>

// MODE
#define EASY 0
#define MEDIUM 1
#define HARD 2
#define CRAZY 3

// VARIANT
// normal
// knopki

/* -------------- TO DO --------------

 * una print per il sudoku che faccia vedere quali caselle sono fisse.
    L'optimum sarebbe usare colori diversi, ma per ora ci facciamo
    bastare l'ASCII.

 * funzioni variadiche per cogliere gli input del comando 
    (per passare implicitamente argomenti (tipo la variante base))

 * generazione e autorisoluzione del sudoku
 */


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


// ====================== 1D ARRAY HELPER FUNCTIONS ====================

//char *arrayToCoords(int idx) {};

/* Transorm a set of x,y coordinates into the corresponding
 * index of a monodimensional array representing a 9x9 grid. */
int coordsToArray(int x, int y) {
    return x + y*9;
};


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

/* Set the cell's 'i' variable  to the desired numerical value,
 * provided that the cell is not fixed. */
void setCellValue(Cell *c, int i) {
    if (!c->isfixed) c->i = i;
}


// ======================= SUDOKU-OBJECT EVALUATION FUNCTIONS =======================

/* This function evaluates whether a given array of numbers
 * contains the same number twice.
 * Return values: true if array contains repetitions, false otherwise. */
bool hasDoubles(int *a) {
    for (int i=0; i<9; i++) { //FIXME: O(n^2)
        for (int j=0; j<9; j++) {if (a[i] == a[j]) return false;}
    }
    return true;
}

/* This function evaluates whether a given sudoku row
 * is valid, that is whether it does not contain repeating numbers.
 * Return values: true if row is valid, false otherwise. */
bool evalRow(SudokuBoard *sb, int row) {
    int buf[9];
    
    // Save row content into a temporary array
    for (int i=0; i<9; i++) {
        int row_formula = i + 9*(row-1);
        Cell *c = sb->cells[row_formula];
        buf[i] = c->i;
    }

    return !hasDoubles(buf); 
};

/* This function evaluates whether a given sudoku column
 * is valid, that is whether it does not contain repeating numbers.
 * Return values: true if column is valid, false otherwise. */
bool evalCol(SudokuBoard *sb, int col) {
    int buf [9];
    
    // Save col content into a temporary array
    for (int i=0; i<9; i++) {
        int col_formula = i*9 + (col-1);
        Cell *c = sb->cells[col_formula];
        buf[i] = c->i;
    }

    return !hasDoubles(buf); 
};

/* This function evaluates whether a given sudoku square
 * is valid, that is whether it does not contain repeating numbers.
 * 
 * The squares are numbered horizontally from 1 to 9 starting from the top left corner
 * to the bottom right corner and going down a line when reaching the right limit.
 * 
 * Return values: true if square is valid, false otherwise. */
bool evalSquare(SudokuBoard *sb, int sqr_num) {

    /*  sqr_num:
     * +-------+-------+-------+ 
     * |       |       |       |
     * |   1   |   2   |   3   |
     * |       |       |       |
     * +-------+-------+-------+ 
     * |       |       |       | 
     * |   4   |   5   |   6   | 
     * |       |       |       | 
     * +-------+-------+-------+ 
     * |       |       |       | 
     * |   7   |   8   |   9   | 
     * |       |       |       | 
     * +-------+-------+-------+
     * * * * * * * * * * */

    // Get the top left corner coordinates from the square number
    int row, col;
    switch (sqr_num) {
    case 1:
        row =1; col =1; break;
    case 2:
        row =1; col =4; break;
    case 3:
        row =1; col =7; break;
    case 4:
        row =4; col =1; break;
    case 5:
        row =4; col =4; break;
    case 6:
        row =4; col =4; break;
    case 7:
        row =7; col =1; break;
    case 8:
        row =7; col =4; break;
    case 9:
        row =7; col =7; break;
    }

    // Get the square center's coordinates
    row++; col++;

    // Get the square cells' values
    int buf[9];
    int counter = 0;
    for (signed int xo=(-1); xo<=1; xo++) {
        for (signed int yo=(-1); yo<=1; yo++) {
            int idx = coordsToArray(row+xo, col+yo);
            Cell *c = sb->cells[idx];
            buf[counter] = c->i;
            counter++;
        } 
    }

    return !hasDoubles(buf); 
};

/* Check whether the generated sudoku was generated correctly.
 * Return values: true if sudoku board valid, false otherwise. */
bool evalSudoku(SudokuBoard *sb) {
    for (int i=1; i<=9; i++) {
        if (evalRow(sb, i) && evalCol(sb, i) && evalSquare(sb, i)) {
            continue;
        } else return false;
    };
    return true;
}; 

/* Create a new sudoku game depending on
 * the chosen difficulty level and game variation. */
void generateSudoku(SudokuBoard *sb, int mode) {
    int rn; // 1-9
    int rx, ry; // 0-8
    int given; // n° of given, fixed numbers

    switch (mode) {
    case EASY: given = 32; break;
    case MEDIUM: given = 16; break;
    case HARD: given = 8; break;
    case CRAZY: given = 4; break;
    }

    /* Try to set a random cell with a random value.
     * Try again with new numbers if the cell is not modifiable
     * or if the sudoku rules would be broken. */
    for (int i=0; i < given; i++) {
        while (1) { 
            // Random numerical value
            rn = rand() % (9-1 +1); // rand() % (upper - lower + 1)
            // Random cell
            rx = rand() % (8-0 +1);
            ry = rand() % (8-0 +1);

            Cell *c = sb->cells[coordsToArray(rx, ry)];
            if (!c->isfixed && (c->i != rn)) { 
                setCellValue(c, rn);
                setCellState(c, true);
                
                //if (!evalSudoku(sb)) {continue;}

                break;

            } else continue;
        }
    }
};




/* Solve the given sudoku game automatically. */
// int solveSudoku(SudokuBoard *sb) {};



// ======================== BOARD PRINTING FUNCTIONS ======================

    /* +-------+-------+-------+ 
     * | 1 2 3 | 4 5 6 | 7 8 9 | +0*9
     * | 1 2 3 | 4 5 6 | 7 8 9 | +1*9
     * | 1 2 3 | 4 5 6 | 7 8 9 | +2*9
     * +-------+-------+-------+ 
     * | 1 2 3 | 4 5 6 | 7 8 9 | 3
     * | 1 2 3 | 4 5 6 | 7 8 9 | 4
     * | 1 2 3 | 4 5 6 | 7 8 9 | 5
     * +-------+-------+-------+ 
     * | 1 2 3 | 4 5 6 | 7 8 9 | 6
     * | 1 2 3 | 4 5 6 | 7 8 9 | 7
     * | 1 2 3 | 4 5 6 | 7 8 9 | 8
     * +-------+-------+-------+
     * * * * * * * * * * */
    
#define TAB_NUM 1
#define TOP_PADDING 1
/* Print a small version of the board that
 * does not show which cells are fixed. */
void printSmallBoard(SudokuBoard *sb) {
    for (int j=0; j<TOP_PADDING; j++) printf("\n");
    for (int i=0; i<81; i++) {
        
        // Tab
        if ((i % 9 == 0)) {for (int j=0; j<TAB_NUM; j++) printf("\t");}

        // Row separator
        if (i % (9*3) == 0) {
            printf("+-------+-------+-------+\n");
            for (int j=0; j<TAB_NUM; j++) printf("\t");
        } 
    
        // Column separator
        if (i*3 % 9 == 0) printf("|");
        
        // Padding and Value
        printf(" %d", ((sb->cells[i]->i) % 9));

        //Padding
        if (((i+1) % 3 == 0)) printf(" ");
    
        // Column separator and Newline 
        if ((i % 9 == 8)) printf("|\n");
    }

    // Last Row separator
    for (int j=0; j<TAB_NUM; j++) printf("\t");
    printf("+-------+-------+-------+\n"); //+tab
    printf("\n");
};

    /*  1 +-------------+-------------+-------------+ 
     *  2 |  .   .   .  |  .   .   .  |  .   .   .  |    
     *  3 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     *  4 |  .   .   .  |  .   .   .  |  .   .   .  |
     *  5 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     *  6 |  .   .   .  |  .   .   .  |  .   .   .  | 
     *  7 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     *  8 |  .   .   .  |  .   .   .  |  .   .   .  |      
     *  9 +-------------+-------------+-------------+
     * 10 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 11 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 12 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 13 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 14 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 15 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 16 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 17 +-------------+-------------+-------------+
     * 18 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 19 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 20 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 21 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 22 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 23 | .1. .2. .3. | .4. .5. .6. | .7. .8. .9. |
     * 24 |  .   .   .  |  .   .   .  |  .   .   .  | 
     * 25 +-------------+-------------+-------------+
     * * * * * * * * * * */


/* Print a version of the board that shows
 * explicitely which cells are fixed. */     
void printBigBoard(SudokuBoard *sb) { // FIXME: adaptate
    for (int j=0; j<TOP_PADDING; j++) printf("\n");
    for (int i=0; i<81; i++) {
        
        // Tab
        if ((i % 9 == 0)) {for (int j=0; j<TAB_NUM; j++) printf("\t");}

        // Row separator
        if (i % (9*3) == 0) {
            printf("+-------------+-------------+-------------+\n");
            for (int j=0; j<TAB_NUM; j++) printf("\t");
            printf("|             |             |             |\n");
            for (int j=0; j<TAB_NUM; j++) printf("\t");
        } 
    
        // Column separator
        if (i*3 % 9 == 0) printf("|");
        
        // Value
        printf("  %d", ((sb->cells[i]->i) % 9));

        // isfixed() character and Padding
        if (sb->cells[i]->isfixed == true) {printf(".");}
        else printf(" ");

        //Padding
        if (((i+1) % 3 == 0)) printf(" ");
    
        // Column separator, Newline and Extra row 
        if ((i % 9 == 8)) {
            printf("|\n");
            for (int j=0; j<TAB_NUM; j++) printf("\t");
            printf("|             |             |             |\n");
        }
    }

    // Last Row separator
    for (int j=0; j<TAB_NUM; j++) printf("\t");
    printf("+-------------+-------------+-------------+\n"); //+tab
    printf("\n");
};


// ============================= MAIN =============================

int main(int argc, char **argv) {

    // ======================= READ INITIAL INPUT =====================
    // read from file
    // read input command line
    // Protoype: sudoku ([--variant]) [--mode]

    // Seed rand()
    srand(time(NULL));

    // ============== INITIALIZE THE SUDOKU BOARD ============
    SudokuBoard *sb = createSudokuBoard();
    generateSudoku(sb, EASY);


    // ============== SHOW THE GAME ON SCREEN ============
    printBigBoard(sb);


    // ============== SIVALLETTO SEQUENCE ============
    // Destroy the sudoku board
    free(sb);

    return 0;
}