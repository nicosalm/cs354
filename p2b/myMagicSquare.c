///////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission, CS 354 Spring 2022, Deb Deppeler
////////////////////////////////////////////////////////////////////////////////
   
// add your own File Header information here (as provided in p2A or Commenting guide

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure that represents a magic square
typedef struct {
    int size;           // dimension of the square
    int **magic_square; // pointer to heap allocated magic square
} MagicSquare;

/* TODO:
 * Prompts the user for the magic square's size, reads it,
 * checks if it's an odd number >= 3 (if not display the required
 * error message and exit), and returns the valid number.
 */
int getSize() {
    return 0;   
} 
   
/* TODO:
 * Makes a magic square of size n using the 
 * Siamese magic square algorithm or alternate from assignment 
 * or another valid alorithm that produces a magic square.
 *
 * n is the number of rows and columns
 *
 * returns a pointer to the completed MagicSquare struct.
 */
MagicSquare *generateMagicSquare(int n) {
    return NULL;    
} 

/* TODO:  
 * Opens a new file (or overwrites the existing file)
 * and writes the square in the specified format.
 *
 * magic_square the magic square to write to a file
 * filename the name of the output file
 */
void fileOutputMagicSquare(MagicSquare *magic_square, char *filename) {

}


/* TODO:
 * Generates a magic square of the user specified size and
 * outputs the square to the output filename.
 * 
 * Add description of required CLAs here
 */
int main(int argc, char **argv) {
    // TODO: Check input arguments to get output filename

    // TODO: Get magic square's size from user

    // TODO: Generate the magic square by correctly interpreting 
    //       the algorithm(s) in the write-up or by writing or your own.  
    //       You must confirm that your program produces a 
    //       Magic Sqare as described in the linked Wikipedia page.

    // TODO: Output the magic square
    
    // TODO: free allocated memory

    return 0;
} 

// S23

