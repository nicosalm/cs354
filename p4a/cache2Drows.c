////////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Drows.c
// This File:        cache2Drows.c
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

int arr2D[3000][500]; // global 2D array stored in data segment

int main()
{
    for (int i = 0; i < 3000; i++) // we iterate in row major order
    {
        for (int j = 0; j < 500; j++)
        {
            arr2D[i][j] = i + j; // row + col is assigned to the index
        }
    }

    return 0;
}