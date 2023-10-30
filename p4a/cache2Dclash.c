////////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Dclash.c
// This File:        cache2Dclash.c
// Other Files:      N/A
// Semester:         CS 354 Lecture 003 Fall 2023
// Instructor:       markm (Mark Mansi)
//
// Author:           Nico Salm
// Email:            nbsalm@wisc.edu
// CS Login:         salm
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          NONE
//
// Online sources:   NONE
//////////////////////////// 80 columns wide ///////////////////////////////////

int arr2D[128][8]; // 128 row by 8 col array

int main()
{
    for (int i = 0; i < 100; i++) // iterate 100 times
    {
        for (int row = 0; row < 128; row += 64) // iterate over every 64th row
        {
            for (int col = 0; col < 8; col++) // iterate over every column
            {
                arr2D[row][col] = i + row + col; // iteration + row + col is assigned to the index
            }
        }
    }

    return 0;
}