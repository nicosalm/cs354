////////////////////////////////////////////////////////////////////////////////
// Main File:        cache1D.c
// This File:        cache1D.c
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

int arr[100000]; // global int array stored in data segment

int main()
{
    for (int i = 0; i < 100000; i++)
    {
        arr[i] = i; // iterates over the array and assigns the index to the value
    }

    return 0;
}