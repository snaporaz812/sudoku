#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>  
#include <stdarg.h>
#include <string.h> // strcmp()
#include <time.h>   // rand()
#include <termios.h>
#include <unistd.h> // usleep()
#include <assert.h>


/* -------------- TO DO --------------

 * Migliorare i colori
 
 * creare una barra soprastante che mostri le info di gioco
    (modalità, difficoltà, durata della partita, shortcut per comandi, numeri completati(?))

 * funzioni variadiche per cogliere gli input del comando (su terminale o da file) 
    (per passare implicitamente argomenti (tipo la variante base))

 * Salvare un gioco in un file separato, e dare la possibilità di riprendere
    il gioco da dove lo si ha lasciato.

 * Continuare le funzioni per ctrl+z e ctrl+y

 * FOLLIA: printare una seconda tabella piccola a lato per le annotazioni
*/


// ANSI TERMINAL COLOR ESCAPE SEQUENCES
#define COL_RESET "\x1b[0m"
#define COL_BLUE "\x1b[38;2;0;0;255m"
#define COL_BLACK "\x1b[38;2;0;0;0m" 
#define BACK_COL_BLUE "\x1b[48;2;0;0;255m"
#define BACK_COL_WHITE "\x1b[48;2;255;255;255m"
/* BREAKDOWN: \x1b [ (xx; t; r;g;b) m
 * xx: foreground (38) or background(48)
 * t: 24-bit true color mode (2) --> allows RGB
 * r, g, b: each go from 0-255 <-- COLOR
*/


// DIFFICULTY SETTINGS
enum MODES {
    EASY,
    MEDIUM,
    HARD,
    CRAZY,
    DEBUG = 255
};

// SUDOKU VARIANT 
enum VARIANT {
    CLASSIC,
    KLOPKI,
    KILLER
};

/* PLAYER INPUT;
 *
 * - either WASD or arrows: move through the sudoku cells
 * -- tab / shift+tab: move sideways; enter / shift+enter: move vertically
 * 
 * - numbers: input numbers
 * 
 * - canc: restore cell to 0
 * 
 * - ctrl+q: quit
 * 
 * - ctrl+s: save
 * 
 * - ctrl+y, ctrl+z = redo, undo
*/

enum KEYS { // Actual ASCII character codes
    KEY_BACKSPACE = 0x08,
    KEY_SHIFT_IN = 0x0F,
    KEY_QUIT = 0x11,    // CTRL+Q
    KEY_SAVE = 0x13,    // CTRL+S
    KEY_REDO = 0x19,    // CTRL+Y
    KEY_UNDO = 0x1A,    // CTRL+Z
    KEY_ESC = 0x1B,
    KEY_DEL = 0x7F      // CANC
};

enum ARROWS { // Arbitrary values
    KEY_UP = -1,
    KEY_DOWN = -2,
    KEY_RIGHT = -3,
    KEY_LEFT = -4
};


// ============================= DATA STRUCTURES =============================

typedef struct Cell {
    int i;         // Cell's value
    bool isfixed;  // If true, i cannot be modified by the player

} Cell;

typedef struct SudokuBoard { 
    Cell *cells[81]; // Whole 9x9 grid
} SudokuBoard;

typedef struct Cursor {
    int x,y;
    //int idx; // fixme: non avrebbe senso togliere la classe Cursor e usare solo idx?
} Cursor;

typedef struct Move {
    int idx;
    int oldv, newv; // old and new cell's value
} Move;

typedef struct MoveHistory {
    int current_idx; // Current move in the "moves" array
    struct Move **moves;
    int len;
} MoveHistory;

typedef struct SaveState {
    int mode;
    int variant;
    long time_elapsed;
    SudokuBoard *sb;
    MoveHistory *mh;
} SaveState;


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

/* Transform a set of x,y coordinates into the corresponding
 * index of a monodimensional array representing a 9x9 grid. */
int coordsToIndex(int x, int y) {
    return (x-1) + 9*(y-1);
};

/* This function takes as input an index to transform in a pair of coordinates,
 * and an interger array to store the coordinates. */
void indexToCoords(int idx, int* coords) {
    coords[0] = idx % 9 + 1;
    coords[1] = idx / 9 + 1;
};

/* | x1 x2 x3 x4 x5 x6 x7 x8 x9 
---+--------------------------- 
y1 | 00 01 02 03 04 05 06 07 08 
y2 | 09 10 11 12 13 14 15 16 17 
y3 | 18 19 20 21 22 23 24 25 26 
y4 | 27 28 29 30 31 32 33 34 35 
y5 | 36 37 38 39 40 41 42 43 44 
y6 | 45 46 47 48 49 50 51 52 53 
y7 | 54 55 56 57 58 59 60 61 62 
y8 | 63 64 65 66 67 68 69 70 71 
y9 | 72 73 74 75 76 77 78 79 80
*/

// ================================ BOARD OBJECTS ============================
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

// =========================== MOVE-HISTORY OBJECTS AND FUNCTIONS ===========================

MoveHistory *createMoveHistory(void) { // It would make sense to pass the sudokuboard as an argument
    MoveHistory *mh = xmalloc(sizeof(*mh));
    mh->moves = NULL;
    mh->len = 0; 
    mh->current_idx = (-1); // FIXME: not sure about this, as there is no element yet
    return mh;
};

/* Destroy the move history, move by move. */
void destroyMoveHistory(MoveHistory *mh) {
    if (!mh) return;
    for (int i=0; i < mh->len; i++) {
        if (mh->moves[i]) free(mh->moves[i]);
    }
    free(mh->moves);
    mh->moves = NULL;
    free(mh);
};

/* Create a Move instance and initialize its values. */
Move *recordMove(int idx, Cell *cell, int newvalue) {
    Move *m = xmalloc(sizeof(*m));
    m->idx = idx;
    m->oldv = cell->i;
    m->newv = newvalue;
    return m;
};

/* Append the new move as the latest element of the move history. */
void pushMove(MoveHistory *mh, Move *m) {
    Move **new_moves = xrealloc(mh->moves, sizeof(Move*) * (mh->len + 1));
    if (!new_moves) {
        fprintf(stderr, "Error pushing a move to the move record.");
        exit(1);
    }
    mh->moves = new_moves;
    mh->moves[mh->len] = m;
    mh->len++;
    mh->current_idx = mh->len-1;
};

/* Substitute an old move in the move history with a new move,
 * then delete all the moves that followed the old move. 
void overwriteMove(MoveHistory *mh, Move *m, int moves_idx) {
    assert(mh->len > 0);

    // Overwrite move in "moves" array
    mh->moves[moves_idx] = m;

    // Delete any following moves
    for (int i = moves_idx + 1; i < mh->len + 1; i++) mh->moves[i] = NULL;

    // Set current index to latest element
    mh->current_idx = moves_idx;

    // Set new len
    mh->len = moves_idx + 1;
};*/


// ======================= SUDOKU-OBJECT EVALUATION FUNCTIONS =======================

/* This function evaluates whether a given array of numbers
 * contains repetitions of the same number.
 * Return values: true if array contains repetitions, false otherwise. */
bool isValid(int *a) {
    for (int i=0; i<9; i++) {
        if (a[i] == 0) continue; // Ignore empty cells 
        for (int j = i+1; j<9; j++) {
            if (a[i] == a[j]) return false;
        }
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
            int idx = coordsToIndex(start_col+1 + c, start_row+1 + r);
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
    }
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

/* This function accepts as input an integer array and its length,
 * then shuffles randomically its content. */
void shuffle (int *a, int len) {
    for (int i = len-1; i > 0; i--) {
        int rn = rand() % (i+1);
        int lorem = a[i];
        a[i] = a[rn];
        a[rn] = lorem;
    }
};

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

    // Choose next cell randomly
    int digits[] = {1,2,3,4,5,6,7,8,9};
    shuffle(digits, 9);
    
    // Solve sudoku cell
    for (int j=0; j<9; j++) {
        
        sb->cells[target]->i = digits[j];
        if (evalSudoku(sb)) {
            if (solveSudoku(sb)) return true; // Recursion
        }

        sb->cells[target]->i = 0;
    }
    
    return false;
};

/* Clear some cells from a fully solved sudoku.
 * The number of cells to be cleared depends on
 * the desired game mode. */
void generateSudoku(SudokuBoard *sb, int mode) {
    // Create a full, valid sudoku board
    solveSudoku(sb);

    // Leave only the desired amount of cells
    int given;
    switch(mode) {
        case DEBUG: given = 81; break;
        case EASY: given = 33; break;
        case MEDIUM: given = 17; break;
        case HARD: given = 11; break;
        case CRAZY: given = 5; break;
    }
    
    // Remove the correct amount of numbers
    int to_clear = 81 - given;
    
    while (to_clear) {
        int r_idx = rand() % 81;
        if (sb->cells[r_idx]->i != 0) {
            sb->cells[r_idx]->i = 0;
            to_clear--;
        }
    }
    
    // Set the remaining cells to fixed
    for (int i=0; i < 81; i++) {if (sb->cells[i]->i != 0) sb->cells[i]->isfixed = true;}
};


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
 * prints fixed cells in a different color. */
void printSmallBoard(SudokuBoard *sb, Cursor *cur) {
    
    //char *row_numbers =   "  x 1 2 3   4 5 6   7 8 9 x\n";
    char *row_separator = "+-------+-------+-------+\n";
    char *tab = "\t";
    
    // Top blank lines
    for (int j0=0; j0<TOP_PADDING; j0++) {
        printf("\n");
    }

    for (int j=0; j<81; j++) {
        Cell *c = sb->cells[j];
        
        // Tab
        if (j % 9 == 0) {for (int j2=0; j2<TAB_NUM; j2++) {printf("%s", tab);}}

        // Row separator
        if (j % (9*3) == 0) {
            printf("%s", row_separator);
            for (int j2=0; j2<TAB_NUM; j2++) {printf("%s", tab);}
        } 

        // Column separator (+ Column numbers and tab if needed)
        if (j*3 % 9 == 0) printf("|");
        
        // Padding
        printf(" ");

        // Cursor position (Value and Color)
        if ((j == coordsToIndex(cur->x, cur->y))) {
            printf(BACK_COL_WHITE);
            if (c->i == 0) {printf("_");} 
            else {
                if (c->isfixed) {printf("%d", c->i);} // isfixed color formatting
                else printf("%d", (c->i) % 10); // Value
            }   
        // Value (and Color if needed)
        } else if (c->i == 0) { printf("_");}
        else {
            if (c->isfixed) {printf(COL_BLUE); printf("%d", c->i);} // isfixed color formatting
            else printf("%d", ((c->i) % 10)); // Value
        }
        printf("%s", COL_RESET); // color reset

        //Padding
        if (((j+1) % 3 == 0)) printf(" ");
    
        // Column separator and Newline 
        if ((j % 9 == 8)) printf("|\n");
    }

    // Last Row separator
    for (int j0=0; j0<TAB_NUM; j0++) printf("%s", tab);
    printf("%s", row_separator);
    printf("%s", COL_RESET); // Color reset
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


/* Print a version of the board that shows explicitely
 * which cells are fixed by means of ASCII characters. */     
void printBigBoard(SudokuBoard *sb) {
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
        
        // Padding
        printf("  ");

        // Value
        if (sb->cells[i]->i == 0) {printf("_");}
        else {printf("%d", (sb->cells[i]->i));}

        // isfixed character or Padding
        if (sb->cells[i]->isfixed == true) {printf(".");}
        else {printf(" ");}

        // Padding
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

// ============================ READ USER TERMINAL INPUT ==============================

int readKey(void) {
    int c = getchar();

    if (c != KEY_ESC) return c;

    /* Arrow keys return 3 characters in a row:
     * esc, '[' + A/B/C/D. */
     
    // If its not an arrow (that is, ESC), it is not followed by '['. 
    if (getchar() != '[') return KEY_ESC;

    switch (getchar()) {
    case 'A': return KEY_UP;
    case 'B': return KEY_DOWN;
    case 'C': return KEY_RIGHT;
    case 'D':
    case 'Z': // Shift+Tab
        return KEY_LEFT;
    default: return KEY_ESC;
    }
}


// =========================== CURSOR FUNCTIONS ============================

Cursor *createCursor(int x, int y) {
    Cursor *cur = xmalloc(sizeof(*cur));
    cur->x = x;
    cur->y = y;
    return cur;
};

void setCursorIdx(Cursor *cur, int idx) {
    int new_coords[2];
    indexToCoords(idx, new_coords);
    cur->x = new_coords[0];
    cur->y = new_coords[1];
};

/* This function modifies the cursor's index on the 1D array,
 * provided the intended direction keeps the cursor inside
 * the boundaries of the board. */
void moveCursor(Cursor *cur, int direction) {
    int idx = coordsToIndex(cur->x, cur->y);
    int xo = 1;
    int yo = 9;
    int new_idx;
    bool isMovementValid = false;

	switch (direction) {
    // HORIZONTAL MOVEMENT
    case KEY_LEFT:
        xo *= (-1);
    case KEY_RIGHT:
        new_idx = idx + xo;
        // Check if movement doesn't wrap around
        if (idx / 9 == new_idx / 9) isMovementValid = true;
        break;

    // VERTICAL MOVEMENT
    case KEY_UP:
        yo *= (-1);
    case KEY_DOWN:
        new_idx = idx + yo; 
        isMovementValid = true;
        break;
    }

    // Edit the cursor's position only if idx belongs to the 9x9 grid.
    // Wrap-around will not be implemented.
    if ((0 <= new_idx && new_idx < 81) && isMovementValid) setCursorIdx(cur, new_idx);
};


// ========================== PLAYER COMMANDS FUNCTIONS ============================

void undoMove(SudokuBoard *sb, MoveHistory *mh, Cursor *cur) {
    if (mh->current_idx < 0) return; // There is no prior move in history

    Move *m = mh->moves[mh->current_idx];
    sb->cells[m->idx]->i = m->oldv; // Apply old value back to board
    setCursorIdx(cur, m->idx);      // Make cursor follow the move

    mh->current_idx--;
};

void redoMove(SudokuBoard *sb, MoveHistory *mh, Cursor *cur) {
    if (mh->current_idx >= mh->len -1) return; // There is no further move in history

    mh->current_idx++;

    Move *m = mh->moves[mh->current_idx];
    sb->cells[m->idx]->i = m->newv; // Apply newer value to cell 
    setCursorIdx(cur, m->idx);      // Make cursor follow the move
};


/* Create a SaveState object that stores the current game's data.
 * Variables to be saved: SudokuBoard, MoveHistory, MODE, VARIANT, elapsed time. */
SaveState *createSaveState(SudokuBoard *sb, MoveHistory *mh, int mode,
                           int variant) { //FIXME: move to an appropriate position
    SaveState *s = xmalloc(sizeof(*s));
    s->sb = sb;
    s->mh = mh; 
    s->mode = mode;
    s->variant = variant;
    //s->time_elapsed = time_elapsed; //FIXME: implement time
    
    return s;
};

/* Save gamestate into a file to allow the player to continue later
 * without losing progress.
 * Variables to be saved: SudokuBoard, MoveHistory, MODE, VARIANT, elapsed time.
 * Return values: true is game was saved successfully, false otherwise.
*/
bool saveGame(SaveState *s, SudokuBoard *sb, MoveHistory *mh) { //FIXME
    s->sb = sb;
    s->mh = mh;
    //s->time_elapsed = time_elapsed; //FIXME: implement time 
    return true; // Game saved successfully
    return false; // Game couldn't be saved
};


// ============================== TERMINAL FUNCTIONS =========================

static struct termios old_termios;

/* Most notably: this function restores canonic mode. */
void restoreTerminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

// ================================= MAIN =================================

int main(int argc, char **argv) {

    if (argc < 2) { // FIXME
        fprintf(stderr,
                "Usage:\n\tStart new game: %s ([--variant]) [--mode]\n\tContinue game: %s continue (<savefile>)\n",
                *argv, *argv);
        return 1;
    }

    //parse
    //if: file -> load
    //else: new game with user-given variables 

    // ============================ READ INITIAL INPUT ===========================
    //FILE *fp = fopen()
    
    // read from file
    // read input command line

    /* ----- Protoypes: -----
     * NEW GAME: sudoku ([--variant]) [--mode]
     * CONTINUE: sudoku continue (<savefile>)
     * ---------------------- */
    

    // ===================== TL;DR: Don't wait for '\n' to flush =====================

    tcgetattr(STDIN_FILENO, &old_termios);
    atexit(restoreTerminal);

    struct termios new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_iflag &= ~(IXON);

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


    // ============== INITIALIZE THE GAME VARIABLES ================

    //---- Read from user input file/command ---- FIXME: catch them from user input
    int mode = EASY; 
    int variant = CLASSIC; 

    //long long elapsed_time; // if present in file, use variable from file. Else, initialize new
    //-------------------------------------------

    // Seed rand()
    srand(time(NULL));

    SudokuBoard *sb = createSudokuBoard();
    MoveHistory *mh = createMoveHistory();
    //SaveState *s = createSaveState(sb, mh, mode, variant); // FIXME: devo capire come cazzo sistemare il savefiles
    SaveState *s = NULL;
    // if (è stata scelta la modalità new game)
    generateSudoku(sb, mode);
    // else (è stata scelta la modalità continue)
    // loadSaveState(file);

    int coords[2];
    Cursor *cur = createCursor(1,1);

    bool game = true;
    bool won = false;

    // ====================== GAME LOOP =======================

    while (game == true) {
        system("clear");
        printSmallBoard(sb, cur);

        if (isSudokuFull(sb) && evalSudoku(sb)) {
            won = true;
            game = false;
            break;
        }
        
        int idx = coordsToIndex(cur->x, cur->y);
        Cell *cell = sb->cells[idx];
        Move *m = NULL;

        int c = readKey();

        // Input = digit
        if (('0' <= c && c <= '9')) {
            if (!cell->isfixed) {
                m = recordMove(idx, cell, c);
                cell->i = c - '0';
                if (m) pushMove(mh, m);
            }
        } else {
            // Input = key
            switch (c) {
            // MOVEMENT
            case 'w':
            case KEY_UP:
                moveCursor(cur, KEY_UP);
                break;

            case 'a':
            case KEY_LEFT:
                moveCursor(cur, KEY_LEFT);
                break;

            case 's':
            case KEY_DOWN:
            case '\n': // ENTER
            case '\r': // ENTER
                moveCursor(cur, KEY_DOWN);
                break;

            case 'd':
            case KEY_RIGHT:
            case '\t': // TAB
                moveCursor(cur, KEY_RIGHT);
                break;
            
            // COMMANDS // FIXME: implement the missing features
            case KEY_DEL:
            case KEY_BACKSPACE:
                if (!cell->isfixed) {
                    m = recordMove(idx, cell, 0);
                    cell->i = 0;
                    if (m) pushMove(mh, m);
                }
                break;

            case KEY_QUIT: // CTRL+Q
                game = false;
                break;

            case KEY_SAVE: // CTRL+S
                s = createSaveState(sb, mh, mode, variant);
                if (saveGame(s, sb, mh)) { // FIXME: implement time
                    printf("Game saved successfully!\n");
                } else { //FIXME: cornuto e mazziato while saving
                    fprintf(stderr, "Error while saving the game.\n");
                    return 1;
                }
                break;

            case KEY_REDO: // CTRL+Y
                redoMove(sb, mh, cur);
                break;
            case KEY_UNDO: // CTRL+Z
                undoMove(sb, mh, cur);
                break;                
            }
        }
    }

    if (won) {
        printf("You won!\n");
        scanf("Press any key to close the program.\n");
    }

    // ============== SIVALLETTO SEQUENCE ============

    free(s);
    free(cur);
    destroySudokuBoard(sb);
    destroyMoveHistory(mh);

    return 0; // atexit() will run automatically
}