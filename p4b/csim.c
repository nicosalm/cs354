// ADD YOUR HEADER COMMENT

////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2013,2019-2020
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission for Spring 2023
//
////////////////////////////////////////////////////////////////////////////////

/*
 * csim.c:
 * A cache simulator that can replay traces (from Valgrind) and output
 * statistics for the number of hits, misses, and evictions.
 * The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/******************************************************************************/
/* DO NOT MODIFY THESE VARIABLES **********************************************/

// Globals set by command line args.
int b = 0; // number of block (b) bits
int s = 0; // number of set (s) bits
int W = 0; // number of lines per set

// Globals derived from command line args.
int B; // block size in bytes: B = 2^b
int S; // number of sets: S = 2^s

// Global counters to track cache statistics in access_data().
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

// Global to control trace output
int verbosity = 0; // print trace if set
/******************************************************************************/

int lru_counter = 0; // global to track next val for lru

// Type mem_addr_t: Use when dealing with addresses or address masks.
typedef unsigned long long int mem_addr_t;

// Type cache_line_t: Use when dealing with cache lines.
//
// NOTE: we don't need to actually store any data because we are only simulating
// whether an access was a cache hit/miss, not actually using it as a cache.
//
typedef struct cache_line
{
    char valid;
    mem_addr_t tag;
    int lru; // for LRU replacement policy
} cache_line_t;

// Type cache_set_t: Use when dealing with cache sets
// Note: Each set is a pointer to a heap array of one or more cache lines.
typedef cache_line_t *cache_set_t;

// Type cache_t: Use when dealing with the cache.
// Note: A cache is a pointer to a heap array of one or more sets.
typedef cache_set_t *cache_t;

// Create the cache we're simulating.
// Note: A cache is a pointer to a heap array of one or more cache sets.
cache_t cache;

/*
 * generate_bitmask:
 * Generate a bitmask where the least-signficant m bits are 1 and all other bits
 * are 0. For example, generate_bitmask(3) would return 0x00000007.
 */
int generate_bitmask(int m)
{
    return (1ULL << m) - 1; // returns a bitmask with the least significant m bits set to 1
}

/*
 * init_cache:
 * - Initialize the global variables S and B based on s and b.
 * - Allocates the data structure for a cache with S sets and W lines per set.
 * - Initializes all valid bits and tags with 0s.
 */
void init_cache()
{
    S = 1; // initiallizes S to 1
    for (int i = 0; i < s; i++)
    { // calculates the value of S
        S *= 2;
    }
    cache = (cache_t)malloc(sizeof(cache_set_t) * S); // allocates memory for S sets
    for (int i = 0; i < S; i++)
    {                                                                 // iterates through all sets
        *(cache + i) = (cache_set_t)malloc(sizeof(cache_line_t) * W); // allocates memory for W lines in the current set
        for (int j = 0; j < W; j++)
        {                                    // iterates through all lines in the current set
            (*(*(cache + i) + j)).valid = 0; // sets valid bit to 0
            (*(*(cache + i) + j)).tag = 0;   // sets tag bits to 0
            (*(*(cache + i) + j)).lru = 0;   // sets the lru count of the line to 0
        }
    }
}

/*
 * free_cache:
 * Frees all heap allocated memory used by the cache.
 */
void free_cache()
{
    for (int i = S - 1; i >= 0; i--)
    { // iterates backwards through all sets
        free(cache[i]);
        cache[i] = 0; // frees the memory each set is pointing and sets it to NULL
    }
    free(cache);
    cache = 0; // frees the mem cach is pointing to and sets it to NULL
}

/*

 * access_data:
 * Simulates data access at given "addr" memory address in the cache.
 *
 * If already in cache, increment hit_cnt
 * If not in cache, cache it (set tag), increment miss_cnt
 * If a line is evicted, increment evict_cnt
 */
void access_data(mem_addr_t addr)
{
    // get set and tag
    mem_addr_t tag = addr >> (s + b);
    int set = (addr >> b) & generate_bitmask(s);

    // Hit
    for (int i = 0; i < W; i++) // W is the associativity of the cache
    {
        if (cache[set][i].valid && cache[set][i].tag == tag) // If the line is valid and the tag matches, it is a hit
        {
            hit_cnt++;                         // Increment the hit count
            cache[set][i].lru = lru_counter++; // Update the lru count
            if (verbosity)                     // If verbosity is set, print hit
                printf("hit ");
            return; // Stop searching for a hit
        }
    }

    // Miss
    miss_cnt++;
    if (verbosity)
        printf("miss "); // Always print miss when it is a miss

    // Find an empty line or the LRU line for possible eviction
    int empty_line = -1;
    int lru_line = 0;
    // Iterate through all lines in the set
    for (int i = 0; i < W; i++)
    {
        if (!cache[set][i].valid) // If an empty line is found, use it
        {
            empty_line = i; // Store the index of the empty line
            break;          // Stop searching if an empty line is found
        }
        else if (cache[set][i].lru < cache[set][lru_line].lru) // If the current line is less recently used than the current LRU line, update the LRU line
        {
            lru_line = i; // Store the index of the LRU line
        }
    }

    // If an empty line is found, use it
    if (empty_line != -1)
    {
        cache[set][empty_line].valid = 1;           // Set the valid bit to 1
        cache[set][empty_line].tag = tag;           // Set the tag bits to the tag
        cache[set][empty_line].lru = lru_counter++; // Set the lru count to the current lru counter
        return;
    }

    // Eviction
    evict_cnt++;
    if (verbosity)
        printf("eviction ");

    // Evict the LRU line
    cache[set][lru_line].tag = tag;
    cache[set][lru_line].lru = lru_counter++;
}

/*
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Reads the input trace file line by line.
 * Extracts the type of each memory access : L/S/M
 * TRANSLATE each "L" as a load i.e. 1 memory access
 * TRANSLATE each "S" as a store i.e. 1 memory access
 * TRANSLATE each "M" as a load followed by a store i.e. 2 memory accesses
 */
void replay_trace(char *trace_fn)
{
    char buf[1000];
    mem_addr_t addr = 0;                   // address read from input trace
    unsigned int len = 0;                  // length read from input trace
    FILE *trace_fp = fopen(trace_fn, "r"); // open trace file for reading

    if (!trace_fp)
    {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while (fgets(buf, 1000, trace_fp) != NULL)
    {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M')
        {
            sscanf(buf + 3, "%llx,%u", &addr, &len);

            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

            switch (buf[1])
            {
            case 'L':
            case 'S':
                access_data(addr);
                break;
            case 'M':
                access_data(addr);
                access_data(addr);
                break;
            }

            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}

/*
 * print_usage:
 * Print information on how to use csim to standard output.
 */
void print_usage(char *argv[])
{
    printf("Usage: %s [-hv] -s <num> -W <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of s bits for set index.\n");
    printf("  -W <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of b bits for block offsets.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -W 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -W 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * print_summary:
 * Prints a summary of the cache simulation statistics to a file.
 */
void print_summary(int hits, int misses, int evictions)
{
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE *output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/*
 * main:
 * Main parses command line args, makes the cache, replays the memory accesses
 * free the cache and print the summary statistics.
 */
int main(int argc, char *argv[])
{
    char *trace_file = NULL;
    char c;

    // Parse the command line arguments: -h, -v, -s, -W, -b, -t
    while ((c = getopt(argc, argv, "s:W:b:t:vh")) != -1)
    {
        switch (c)
        {
        case 'b':
            b = atoi(optarg);
            break;
        case 'W':
            W = atoi(optarg);
            break;
        case 'h':
            print_usage(argv);
            exit(0);
        case 's':
            s = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        default:
            print_usage(argv);
            exit(1);
        }
    }

    // Make sure that all required command line args were specified.
    if (s == 0 || W == 0 || b == 0 || trace_file == NULL)
    {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    // Initialize cache.
    init_cache();

    // Replay the memory access trace.
    replay_trace(trace_file);

    // Free memory allocated for cache.
    free_cache();

    // Print the statistics to a file.
    // DO NOT REMOVE: This function must be called for test_csim to work.
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}

// Spring 202301
