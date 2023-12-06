#include <stdio.h>
#include <stdlib.h>

/**
 * Given an array of integers, return indices of the two numbers such that they add up to a specific target.
 * You may assume that each input would have exactly one solution.
 */

/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* twoSum(int* nums, int numsSize, int target, int* returnSize) {

    *returnSize = 2;
    int *result = (int *)malloc(sizeof(int) * (*returnSize));

    for (int i = 0; i < numsSize; i++) {
        for (int j = i + 1; j < numsSize; j++) {
            if (target == nums[i] + nums[j]) {
                result[0] = i;
                result[1] = j;
                return result;
            }
        }
    }

    return result;
}

/**
 * Example:
 * Given nums = [2, 7, 11, 15], target = 9,
 * Because nums[0] + nums[1] = 2 + 7 = 9, return [0, 1].
 */
int main() {
    int nums[] = {2, 7, 11, 15};
    int target = 9;
    int numsSize = sizeof(nums) / sizeof(nums[0]);
    int *result = twoSum(nums, numsSize, target, &numsSize);
    printf("[%d, %d]\n", result[0], result[1]);
    free(result);
    return 0;
}
