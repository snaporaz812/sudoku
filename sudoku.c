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
    int i;          // Cell's value
    bool isfixed;   // If true, i cannot be modified by the player
} Cell;

typedef struct SudokuBoard { 
    Cell *cells[81]; // Whole 9x9 grid
} SudokuBoard;


// =========================== ALLOCATION WRAPPERS =========================

// Try to allocate n bytes, else close the program.
void *xmalloc(size_t size) {
	void *ptr = malloc(size);
	if (ptr == NULL) {
		fprintf(stderr, "Out of memory allocating %zu bytes\n", size);
		exit(1);
	}
	return ptr;
}

// Try to reallocate n bytes, else close the program.
void *xrealloc(void *oldptr, size_t size) {
	void *ptr = realloc(oldptr, size);
	if (ptr == NULL) {
		fprintf(stderr, "Out of memory reallocating %zu bytes\n", size);
		exit(1);
	}
	return ptr;
};


// ====================== 1D ARRAY HELPER FUNCTIONS ==========================

/* Transorm a set of x,y coordinates into the corresponding
 * index of a monodimensional array representing a 9x9 grid. */
int coordsToArray(int x, int y) {
    return x + y*9;
};


// ============================= BOARD OBJECTS ============================
/* Allocate and initialize objects. */

Cell *createCell(int value, bool isfixed) {
    Cell *c = xmalloc(sizeof(*c));
    c->i = value;
    c->isfixed = isfixed;
    return c;
};

SudokuBoard *createSudokuBoard(void) {
    SudokuBoard *sb = xmalloc(sizeof(*sb));

    // Initialize cells
    for (int i=0; i < 81; i++) {    
        sb->cells[i] = createCell(0, false);
    }

    return sb;
};

/* Destroy a sudoku board and all its cells. */
void destroySudokuBoard(SudokuBoard *sb) {
    for (int j=0; j<81; j++) {
        Cell *c = sb->cells[j];
        
        free(c);
        c = NULL;   
    }

    free(sb);
};


// ======================= SUDOKU-OBJECT EVALUATION FUNCTIONS =======================

/* This function evaluates whether a given array of numbers
 * contains repetitions of the same number.
 * Return values: true if array contains repetitions, false otherwise. */
bool isValid(int *a) {
    for (int i=0; i<9; i++) { //FIXME: O(n^2)
        if (a[i] == 0) continue; // Ignore empty cells 
        for (int j = i+1; j<9; j++) {
            if (a[i] == a[j]) return true;
        }
    }
    return false;
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

    return isValid(buf); 
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

    return isValid(buf); 
};

/* This function evaluates whether a given sudoku square
 * is valid, that is whether it does not contain repeating numbers.
 * 
 * The squares are numbered horizontally from 1 to 9 starting from the top left corner
 * to the bottom right corner and going down a line when reaching the right limit.
 * 
 * Return values: true if square is valid, false otherwise. */
bool evalSquare(SudokuBoard *sb, int sqr_num) {
    // Top-left row and col for each 3x3 subgrid (0-indexed)
    int start_row = ((sqr_num - 1) / 3) * 3;
    int start_col = ((sqr_num - 1) % 3) * 3;

    int buf[9];
    int counter = 0;

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int idx = coordsToArray(start_col + c, start_row + r);
            buf[counter++] = sb->cells[idx]->i;
        }
    }

    return isValid(buf); 
}

/* Check whether the generated sudoku was generated correctly.
 * Return values: true if sudoku board is valid, false otherwise. */
bool evalSudoku(SudokuBoard *sb) {
    for (int i=1; i<=9; i++) {
        if (evalRow(sb, i) && evalCol(sb, i) && evalSquare(sb, i)) {
            continue;
        } else return false;
    };
    return true;
};

/* Return true if all sudoku cells are different from zero,
 * otherwise return false. */
bool isSudokuFull(SudokuBoard *sb) {
    for (int i=0; i < 81; i++) {
        if (sb->cells[i]->i == 0) return false;
    }
    return true;
}

// =========================== SUDOKU CREATION ============================ 

/* Fill in a sudoku board according to the sudoku rules.
 * The initial sudoku board must have at least one cell set
 * to 0 for the algorhythm to work propertly.
 * Return values: true if the sudoku is solved, false otherwise. */
bool solveSudoku(SudokuBoard *sb) {
    // Find the first cell set to 0 in the array
    int target = -1;
    for (int j=0; j < 81; j++) {
        if (sb->cells[j]->i == 0) {
            target = j;
            break;
        }
    }

    // No cell is set to zero --> sudoku is solved and valid
    if (target == -1) return true;


    
    for (int j=1; j<=9; j++) {

        sb->cells[target]->i = j;
        if (evalSudoku(sb)) {
            if (solveSudoku(sb)) return true;
        }

        sb->cells[target]->i = 0;
    }
    
    return false;
};

/* Create a new sudoku game depending on
 * the chosen difficulty level and game variation. */
void generateSudoku(SudokuBoard *sb, int mode) {
    // Create a full, valid sudoku board
    solveSudoku(sb);

    // Leave only the desired amount of cells
    int given;
    switch(mode) {
        case EASY: given = 33; break;
        case MEDIUM: given = 17; break;
        case HARD: given = 11; break;
        case CRAZY: given = 5; break;
    }
    
    for (int j=0; j < (81 - given); j++) {
        int r_idx = rand() % 81;
        sb->cells[r_idx]->i = 0; 
    }

    // Set remaining cells to fixed
    for (int i=0; i < 81; i++) {if (sb->cells[i] != 0) sb->cells[i]->isfixed = true;}
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
    char *row_separator = "+-------+-------+-------+\n";
    char *tab = "\t";
    for (int j=0; j<TOP_PADDING; j++) printf("\n");
    for (int i=0; i<81; i++) {
        
        // Tab
        if ((i % 9 == 0)) {for (int j=0; j<TAB_NUM; j++) printf("%s", tab);}

        // Row separator
        if (i % (9*3) == 0) {
            printf("%s", row_separator);
            for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
        } 
    
        // Column separator
        if (i*3 % 9 == 0) printf("|");
        
        // Padding and Value
        if (sb->cells[i]->i == 0) {printf(" _");}
        else {printf(" %d", ((sb->cells[i]->i) % 10));}

        //Padding
        if (((i+1) % 3 == 0)) printf(" ");
    
        // Column separator and Newline 
        if ((i % 9 == 8)) printf("|\n");
    }

    // Last Row separator
    for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
    printf("%s", row_separator);
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
    char *row_separator = "+-------------+-------------+-------------+\n";
    char *row_spacing = "|             |             |             |\n";
    char *tab = "\t";


    for (int j=0; j<TOP_PADDING; j++) printf("\n");
    for (int i=0; i<81; i++) {
        
        // Tab
        if ((i % 9 == 0)) {for (int j=0; j<TAB_NUM; j++) printf("%s", tab);}

        // Row separator
        if (i % (9*3) == 0) {
            printf("%s", row_separator);
            for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
            printf("%s", row_spacing);
            for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
        } 
    
        // Column separator
        if (i*3 % 9 == 0) printf("|");
        
        // Value
        if (sb->cells[i]->i == 0) {printf("  _");}
        else {printf("  %d", (sb->cells[i]->i));}

        // isfixed() character and Padding
        if (sb->cells[i]->isfixed == true) {printf(".");}
        else {printf(" ");}

        //Padding
        if (((i+1) % 3 == 0)) printf(" ");
    
        // Column separator, Newline and Extra row 
        if ((i % 9 == 8)) {
            printf("|\n");
            for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
            printf("%s", row_spacing);
        }
    }

    // Last Row separator
    for (int j=0; j<TAB_NUM; j++) printf("%s", tab);
    printf("%s", row_separator); //+tab
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
    solveSudoku(sb);


    // ============== SHOW THE GAME ON SCREEN ============
    printBigBoard(sb);


    // ============== SIVALLETTO SEQUENCE ============
    // Destroy the sudoku board
    destroySudokuBoard(sb);

    return 0;
}