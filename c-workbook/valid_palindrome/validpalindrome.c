#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/**
 * @brief Check if a string is a palindrome
 *
 * @param s string to check
 * @return true if palindrome, false otherwise
 */
bool isPalindrome(char *s) {
    char* start = s;
    char* end = s + strlen(s) - 1;

    while (start <= end) {
        if (!isalnum(*start)) {
            start++;
            continue;
        }
        if (!isalnum(*end)) {
            end--;
            continue;
        }
        if (tolower(*start) != tolower(*end)) {
            return false;
        }
        start++;
        end--;
    }
    return true;
}

/**
 * We check if the string is a palindrome by comparing the first and last
 * characters. If they are not the same, we return false. If they are the same,
 * we move the pointers to the next character and repeat the process.
 *
 * @see isPalindrome
 *
 * @note This is a leetcode problem
 * @see https://leetcode.com/problems/valid-palindrome/
 *
 * We check if "taco cat" is a palindrome
 */
int main() {
    char* s = "taco cat";
    bool result = isPalindrome(s);
    printf("%s\n", result ? "true" : "false");
    return 0;

}
