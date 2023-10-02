#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int arr[2][4] = {{0,1,2,3},{4,5,6,7}};
    int *arrflat = (int*)arr;   // we can do this because the array is stored in memory as a contiguous block
                                // we cast it to an int pointer so we can iterate over it
    for (int i = 0; i < 8; ++i) {
        printf("%d ", arrflat[i]);
    }
    printf("\n");
}

void io() {
    int a;
    float f;
    printf("Enter an int and a float:\n");
    int nsuccess = scanf("%d %f", &a, &f);
    printf("You entered %d and %f\n", a, f);
    if (nsuccess != 2) {
        perror("You didn't enter two numbers!\n");
    }
}

void fileio() {
    FILE *fp = fopen("test.txt", "r");
    if (fp == NULL) {
        perror("Couldn't open file!\n");
        exit(1);
    }
    char buf[100];
    while (fgets(buf, 100, fp) != NULL) {
        printf("%s", buf);
    }
    fclose(fp);
}