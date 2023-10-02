///////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission, CS 354 Spring 2022, Deb Deppeler
////////////////////////////////////////////////////////////////////////////////
// Main File:        myMagicSquare.c
// This File:        myMagicSquare.c
// Other Files:      check_board.c
// Semester:         CS 354 Fall 2023
// Instructor:       Mark Mansi
//
// Author:           Nico Salm
// Email:            nbsalm@wisc.edu
// CS Login:         salm
// GG#:              n/a
//                   (See https://canvas.wisc.edu/groups for your GG number)
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   Fully acknowledge and credit all sources of help,
//                   including family, friends, classmates, tutors,
//                   Peer Mentors, TAs, and Instructor.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the ideas and help they provided.
//
// Online sources:   Avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description
//                   of any information you find.
////////////////////////////////////////////////////////////////////////////////
// add your own File Header information here (as provided in p2A or Commenting guide

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure that represents a magic square
typedef struct
{
    int size;           // dimension of the square
    int **magic_square; // pointer to heap allocated magic square
} MagicSquare;

/* TODO:
 * Prompts the user for the magic square's size, reads it,
 * checks if it's an odd number >= 3 (if not display the required
 * error message and exit), and returns the valid number.
 */
int getSize()
{
    int size;

    printf("Enter magic square's size (odd integer >=3)\n");
    scanf("%d", &size);
    // break into two if statements
    // first check if size is odd
    if (size % 2 == 0)
    {
        printf("Size must be odd.\n");
        exit(1);
    }
    // then check if size is >= 3
    if (size < 3) {
        printf("Size must be an integer >= 3.\n");
        exit(1);
    }
    return size;
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
MagicSquare *generateMagicSquare(int n)
{
    int **magic_square = malloc(n * sizeof(int *));
    if (magic_square == NULL)
    {
        printf("Error allocating memory for magic square.\n");
        exit(1);
    }
    // use pointer arithmetic to allocate memory for each row and initialize all values to 0; no [] allowed
    for (int i = 0; i < n; i++)
    {
        *(magic_square + i) = calloc(n, sizeof(int));
        if (*(magic_square + i) == NULL)
        {
            printf("Error allocating memory for magic square.\n");
            exit(1);
        }
    }
    // initialize variables
    int row = 0;
    int col = n / 2;
    int next_row;
    int next_col;
    int next_num = 1;
    // loop through all numbers from 1 to n^2
    while (next_num <= n * n)
    {
        // set the current position to the next number
        *(*(magic_square + row) + col) = next_num;
        // set the next position
        next_row = row - 1;
        next_col = col + 1;
        // if the next row is out of bounds, wrap around to the bottom
        if (next_row < 0)
        {
            next_row = n - 1;
        }
        // if the next column is out of bounds, wrap around to the left
        if (next_col >= n)
        {
            next_col = 0;
        }
        // if the next position is already filled, wrap around to the bottom
        if (*(*(magic_square + next_row) + next_col) != 0)
        {
            next_row = row + 1;
            next_col = col;
        }
        // set the next position
        row = next_row;
        col = next_col;
        next_num++;
    }

    // create a MagicSquare struct and set its values
    MagicSquare *magic_square_struct = malloc(sizeof(MagicSquare));
    if (magic_square_struct == NULL)
    {
        printf("Error allocating memory for magic square.\n");
        exit(1);
    }
    magic_square_struct->size = n;
    magic_square_struct->magic_square = magic_square;
    return magic_square_struct;
}

/* TODO:
 * Opens a new file (or overwrites the existing file)
 * and writes the square in the specified format.
 *
 * magic_square the magic square to write to a file
 * filename the name of the output file
 */
void fileOutputMagicSquare(MagicSquare *magic_square, char *filename)
{
    // open a new file for writing (or overwrite an existing file)
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error opening file for writing.\n");
        exit(1);
    }
    // write the magic square to the file. the first line of the file is the size of the square.
    // Every line after that represents a row in the matrix, starting with the first row. There will be n such lines where each line has n numbers (columns) separated by commas.

    // write the size of the square to the file
    fprintf(file, "%d\n", magic_square->size);
    // write each row of the square to the file
    for (int i = 0; i < magic_square->size; i++)
    {
        for (int j = 0; j < magic_square->size; j++)
        {
            // write the number to the file
            fprintf(file, "%d", *(*(magic_square->magic_square + i) + j));
            // if the number is not the last number in the row, write a comma
            if (j != magic_square->size - 1)
            {
                fprintf(file, ",");
            }
        }
        // if the row is not the last row, write a newline
        if (i != magic_square->size - 1)
        {
            fprintf(file, "\n");
        }
    }
    // close the file
    fclose(file);
}

/* TODO:
 * Generates a magic square of the user specified size and
 * outputs the square to the output filename.
 *
 * Add description of required CLAs here
 */
int main(int argc, char **argv)
{
    // Check input arguments to get output filename
    if (argc != 2)
    {
        printf("Usage: ./myMagicSquare <output_filename>\n");
        exit(1);
    }

    // Get magic square's size from user
    int size = getSize();

    // Generate the magic square by correctly interpreting
    //       the algorithm(s) in the write-up or by writing or your own.
    //       You must confirm that your program produces a
    //       Magic Square as described in the linked Wikipedia page.

    MagicSquare *magic_square = generateMagicSquare(size);

    // Output the magic square
    fileOutputMagicSquare(magic_square, argv[1]);

    // free allocated memory
    for (int i = 0; i < size; i++)
    {
        free(*(magic_square->magic_square + i));
    }
    free(magic_square->magic_square);
    free(magic_square);

    return 0;
}

// S23
