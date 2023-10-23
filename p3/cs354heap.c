///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2023 Mark Mansi, Deb Deppeler based on work by Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission FALL 2023, CS354-markm
//
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Main File:        cs354heap.c
// This File:        cs354heap.c
// Other Files:      None
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
// Persons:          NONE
//
// Online sources:   NONE
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cs354heap.h"

///////////////////////////////////////////////////////////////////////////////
// Define some useful constants and types
//
// You are free to change these definitions as you wish or add others.
///////////////////////////////////////////////////////////////////////////////

/*
 * The minimum size in bytes for the payload of a block. This is determined by
 * the size of a block footer because the footer needs to fit entirely inside
 * the payload section of a free block.
 */
#define MIN_PAYLOAD_SIZE (round_up_block_size(sizeof(BlockFooter)))

/*
 * The minimum total size of a block in bytes, including the header, footer,
 * freelist pointers, payload, and padding for alignment.
 */
#define MIN_BLOCK_SIZE (sizeof(BlockHeader) + MIN_PAYLOAD_SIZE)

/*
 * A bitmask to help us get the p-bit from header blocks.
 */
#define P_BITMASK (1 << 0)

/*
 * A bitmask to help us get the a-bit from header blocks.
 */
#define A_BITMASK (1 << 1)

/*
 * A bitmask to help us get the size from header blocks.
 */
#define SIZE_BITMASK 0xFFFFFFF8

/*
 * The A-bit value if the block is free.
 */
#define ABIT_FREE 0

/*
 * The A-bit value if the block is allocated.
 */
#define ABIT_USED A_BITMASK

/*
 * The P-bit value if the previous block is free.
 */
#define PBIT_FREE 0

/*
 * The P-bit value if the block is allocated.
 */
#define PBIT_USED P_BITMASK

/*
 * The end mark alone has the following contents for its size_status value.
 * Notice that bit 2 is set, which should never happen for other blocks.
 *
 * We use a special recognizable value like this so that if we see it while
 * debugging, it's easily recognizable.
 */
#define END_MARK_MAGIC (0xDEADFFFC | ABIT_USED)

///////////////////////////////////////////////////////////////////////////////
// Define the block header structure we will use throughout the heap impl.
//
// DO NOT change these struct definitions, as our tests make assumptions about
// the size of a block.
///////////////////////////////////////////////////////////////////////////////

/*
 * This structure serves as the header for all blocks, allocated or free.
 *
 * On a 32-bit machine, this struct should be 8B. That allows us to have less
 * math to do to keep all blocks 8B-aligned.
 */
typedef struct BlockHeader
{
    // The size and status bits of this block header.
    //
    // NOTE: Be careful when working with the size: be consistent! Is the size
    // representing the whole block? Or just the payload?
    //
    // Recall that size_t is an unsigned integer type that is large enough to
    // represent any possible amount of memory we could want.
    //
    // The bits are used as followed:
    //
    // 31               16 15                0
    // SSSS SSSS SSSS SSSS SSSS SSSS SSSS S0AP
    //
    // Bits 31 to 3: the block size (always divisible by 8)
    // Bit        2: unused, always 0 except for the end mark
    // Bit        1: the A bit -- 0 if this block is free; 1 if used
    // Bit        0: the P bit -- 0 if prev block is free; 1 if used
    //
    // To avoid confusion about which bit is which and whether 1 is free or used,
    // we have defined a few constants for you above:
    //
    // - SIZE_BITMASK, P_BITMASK, A_BITMASK: these are "bitmasks" -- you can use
    //    them with bitwise operations to extract only the bits you want from
    //    size_status. For example,
    //
    //        header->size_status & SIZE_BITMASK
    //        header->size_status & P_BITMASK
    //
    // - ABIT_FREE, ABIT_USED -- these are integers with the right bit set or not
    //    set. You can use them to set the correct value when writing to block
    //    headers, assuming that we start with a header that has the bit cleared
    //    to begin with. For example,
    //
    //        header->size_status |= ABIT_USED
    //
    // - PBIT_FREE, PBIT_USED -- same, but for the P-bit.
    //
    size_t size_status;

    // Unused, only makes the header 8B, so that we have less math to do.
    size_t padding;
} BlockHeader;

/*
 * This structure serves as the footer for free blocks. Note that only free
 * blocks have a footer. This structure fits at the end of the payload space
 * when the block is unallocated. It indicates the size, allowing us to quickly
 * find the header of the previous block to coallesce blocks when freeing; we
 * won't actually coallesce on free in this assignment, even though we will
 * implement the needed infrastructure to do it.
 *
 * We will also will have the free list pointers in the footer, since they are
 * also only needed for free blocks.
 *
 * We could have put these pointers in the BlockHeader, but that would
 * complicate the code a bit because then the meaning of those header fields
 * would vary based on whether the block is free. The footer only exists in free
 * blocks, so it avoids this problem. If you are curious, an alternate
 * implementation use a union. Something like this:
 *
 * struct BlockHeader {
 *     size_t size_status;
 *     union {
 *         // when used, this is the first byte of the payload, so
 *         // &header->payload is the payload address
 *         char payload[1];
 *
 *         // when free, this struct is the free list pointers and also lives at
 *         // the beginning of the "payload" area.
 *         struct {
 *             BlockHeader *prev_block;
 *             BlockHeader *next_block;
 *         }
 *     }
 * };
 */
typedef struct BlockFooter
{
    struct BlockFooter *free_list_prev;
    struct BlockFooter *free_list_next;
    size_t size;
} BlockFooter;

///////////////////////////////////////////////////////////////////////////////
// Define global variables for our heap.
//
// DO NOT change the existing variables. You may define your own additional one
// if you wish.
///////////////////////////////////////////////////////////////////////////////

/*
 * Global variable - DO NOT CHANGE NAME or TYPE.
 * It must point to the first block in the heap and is set by init_heap()
 * i.e., the block at the lowest address.
 */
BlockHeader *heap_start = NULL;

/*
 * Size of heap memory that was allocated to the heap. The actual amount of
 * memory usable will be less than that do to the end mark and the block
 * headers.
 */
size_t heap_size = 0xFFFFFFFF;

/*
 * A pointer to the beginning of the explicit free list. Note that here we are
 * creating the free list using the BlockFooter struct. See the comment on
 * BlockFooter.
 */
BlockFooter *free_list = NULL;

///////////////////////////////////////////////////////////////////////////////
// Define some useful functions for manipulating blocks.
//
// You DO NOT HAVE TO IMPLEMENT OR USE these functions. We will only test
// your implementation of balloc and bfree. You can implement them however you
// wish; these are just suggestions we think you will find useful.
//
// Not all of these functions will be used in PART A. Feel free to disregard any
// functions in this section.
//
// We define these as "static" functions, which basically just means they are
// only available in this file; you can't call them elsewhere. This helps us
// contain implementation details to this file.
///////////////////////////////////////////////////////////////////////////////

/*
 * Returns a true (i.e. non-zero) value if the block indicated by header is
 * actually the end mark (i.e., the fake block that indicates the end of the
 * heap).
 *
 * Parameters:
 * - header: the header to check
 *
 * Returns:
 * - true if and only if header is actually the end mark
 */
static int is_end_mark(BlockHeader *header)
{
    // We need to explicitly ignore the p-bit because adjacent blocks may set it
    // as they are freed/allocated.
    return (header->size_status & ~P_BITMASK) == END_MARK_MAGIC;
}

/*
 * Set the header of a block.
 *
 * Parameters:
 * - header: a pointer to the header we want to change.
 * - size: the payload size of the block (i.e., size excluding the 8B header)
 * - abit: the value of the a-bit, shifted by the right amount. It's recommended
 *         to always call this function with either ABIT_FREE or ABIT_USED.
 * - pbit: the value of the p-bit, shifted by the right amount. It's recommended
 *         to always call this function with either PBIT_FREE or PBIT_USED.
 */
static void set_block_header(BlockHeader *header, size_t size, int abit, int pbit)
{
    // Some sanity checking
    assert((size % 8) == 0);
    assert((abit & ~A_BITMASK) == 0);
    assert((pbit & ~P_BITMASK) == 0);

    // set the size and status
    header->size_status = size | abit | pbit;
}

/*
 * Extract the block size from the header.
 *
 * Parameters:
 * - header: the header for the block we want to get the size of.
 *
 * Return:
 * - the size of the block
 */
static size_t get_block_size(BlockHeader *header)
{
    return header->size_status & SIZE_BITMASK;
}

/*
 * Extract the A-bit from the header.
 *
 * Parameters:
 * - header: the header for the block we want to get the A-bit of.
 *
 * Return:
 * - the A-bit of the block. That is, the value returned will be either
 *   ABIT_FREE or ABIT_USED.
 */
static int get_block_abit(BlockHeader *header)
{
    return (header->size_status & A_BITMASK) ? ABIT_USED : ABIT_FREE;
}

/*
 * Set the A-bit in the given header, while leaving the rest of the header
 * untouched.
 *
 * Parameters:
 * - header: the header to set the A-bit of
 * - abit: the value of the a-bit, shifted by the right amount. It's recommended
 *         to always call this function with either ABIT_FREE or ABIT_USED.
 */
static void set_block_abit(BlockHeader *header, int abit)
{
    if (abit == ABIT_FREE)
    {
        header->size_status &= ~ABIT_USED;
    }
    else
    {
        header->size_status |= ABIT_USED;
    }
}

/*
 * Extract the P-bit from the header.
 *
 * Parameters:
 * - header: the header for the block we want to get the P-bit of.
 *
 * Return:
 * - the P-bit of the block. That is, the value returned will be either
 *   PBIT_FREE or PBIT_USED.
 */
static int get_block_pbit(BlockHeader *header)
{
    return (header->size_status & P_BITMASK) ? PBIT_USED : PBIT_FREE;
}

/*
 * Set the P-bit in the given header, while leaving the rest of the header
 * untouched.
 *
 * Parameters:
 * - header: the header to set the P-bit of
 * - pbit: the value of the p-bit, shifted by the right amount. It's recommended
 *         to always call this function with either PBIT_FREE or PBIT_USED.
 */
static void set_block_pbit(BlockHeader *header, int pbit)
{
    // if the p-bit is free, clear the used bit
    if (pbit == PBIT_FREE)
    {
        header->size_status &= ~PBIT_USED;
    }
    // otherwise, set the used bit
    else
    {
        header->size_status |= PBIT_USED;
    }
}

/*
 * Get a pointer to the payload corresponding to this header.
 *
 * Parameters:
 * - header: the header to get the payload of.
 *
 * Return:
 * - a pointer to the first byte of the payload. In other words, if this block
 *   was allocated, this is the pointer we would return from balloc.
 */
static void *get_block_payload(BlockHeader *header)
{
    return (void *)(header + 1);
}

/*
 * Get the header of the block after this one spatially in the heap (NOT the
 * block next in the free list).
 *
 * For example: suppose our heap looks like this:
 *
 *          -----------
 *          | header1 |
 *          -----------
 *          | payload |
 *          |         |
 *          |   ...   |
 *          -----------
 *          | header2 |
 *          -----------
 *          | payload |
 *          |         |
 *          |   ...   |
 *          -----------
 *
 * Then, passing a pointer to header1 to this function should return a pointer
 * to header2.
 *
 *
 * Parameters:
 * - header: the current block
 *
 * Return:
 * - the block that starts at the address after the end of this block.
 */
static BlockHeader *get_next_adjacent_block(BlockHeader *header)
{
    if (is_end_mark(header))
    {
        return NULL;
    }

    // header + size of block + size of header
    return (BlockHeader *)(((char *)header) + sizeof(BlockHeader) + get_block_size(header));
}

/*
 * Given a block header, return a pointer to the footer of this block.
 *
 * NOTE: the block footer is only valid if the block is free, but you can still
 * find out where it would be if the block were free.
 *
 * Parameters:
 * - header: the block header for which we want the corresponding footer.
 *
 * Returns:
 * - a pointer to the block footer of this block
 */
static BlockFooter *find_block_footer(BlockHeader *header)
{
    // header + size of block + size of header - size of footer
    return (BlockFooter *)(((char *)header) + get_block_size(header) + sizeof(BlockHeader) - sizeof(BlockFooter));
}

/*
 * Given a block footer, return a pointer to the header of this block.
 *
 * NOTE: the footer needs to be valid for this to work, since we read values in
 * the footer to determine where the header must be.
 *
 * Parameters:
 * - footer: a pointer to a valid block footer.
 *
 * Returns:
 * - a pointer to the corresponding block header for the block containing this footer.
 */
static BlockHeader *find_block_header(BlockFooter *footer)
{
    // footer - size of footer - size of header + size of footer
    return (BlockHeader *)(((char *)footer) - footer->size - sizeof(BlockHeader) + sizeof(BlockFooter));
}

/*
 * Similar to get_next_adjacent_block except in the other direction.
 *
 * NOTE: this only works if the previous block is FREE because we need a valid
 * footer! Check your p-bits!
 */
static BlockHeader *get_prev_adjacent_block(BlockHeader *header)
{
    if (get_block_pbit(header) != PBIT_FREE) // check if the previous block is null
    {
        return NULL;
    }
    // get the footer of the previous block and return the header of that block
    BlockFooter *footer = (BlockFooter *)(((char *)header) - sizeof(BlockFooter));
    return find_block_header(footer);
}

/*
 * Set the size in the footer for this block, leaving the rest of the footer untouched.
 *
 * Parameters:
 * - footer: a pointer to the footer we want to modify
 * - size: the size of the block
 */
static void set_block_footer_size(BlockFooter *footer, size_t size)
{
    footer->size = size;
}

/*
 * PART A:
 * Starting at the first heap block, iterate over all blocks in the heap and
 * select a free block whose payload size is closest to the given size, skiping
 * allocated blocks and blocks that are too small.
 *
 * PART B:
 * Starting at the first heap block in the free list, iterate over all blocks in
 * the free list and select a free block whose payload size is closest to the
 * given size, skiping blocks that are too small.
 *
 * Parameters:
 * - size: the requested payload size.
 *
 * Returns:
 * - a pointer to the header of the block whose payload size most closely
 *   matches the requested payload size.
 * - NULL if there is no large-enough block.
 */
static BlockHeader *best_fit_select_block(size_t size)
{
    BlockHeader *curr = heap_start;
    BlockHeader *best_fit = NULL;
    size_t best_size = SIZE_MAX; // largest possible size_t value

    // now this is a simple linear search
    while (!is_end_mark(curr))
    {
        size_t curr_size = get_block_size(curr);

        // check if the block is free and large enough
        if (get_block_abit(curr) == ABIT_FREE && curr_size >= size && curr_size < best_size)
        {
            // if it is, update the best fit
            best_fit = curr;
            best_size = curr_size;
        }

        // move to the next block
        curr = get_next_adjacent_block(curr);
    }

    return best_fit; // NULL if no large-enough block
}

/*
 * Given an integer, round up to the nearest multiple of BLOCK_ALIGNMENT, or
 * return the same integer if it is already aligned.
 *
 * For example, suppose BLOCK_ALIGNMENT is 8, then
 *
 *  round_up_block_size(0) == 0
 *  round_up_block_size(1) == 8
 *  round_up_block_size(2) == 8
 *  round_up_block_size(7) == 8
 *  round_up_block_size(8) == 8
 *  round_up_block_size(9) == 16
 *
 * Parameters:
 * - unrounded: the integer we want to round
 *
 * Returns: the rounded integer
 */
static size_t round_up_block_size(size_t unrounded)
{
    size_t almost_too_far = unrounded + BLOCK_ALIGNMENT - 1;
    size_t remainder = almost_too_far % BLOCK_ALIGNMENT;
    return almost_too_far - remainder;
}

/*
 * Remove the given block from the free list.
 *
 * Be careful! Make sure that the footer is valid and has valid pointers in it!
 *
 * Parameters:
 * - header: the header of a free block to remove from the free list.
 */
static void remove_from_free_list(BlockHeader *header)
{
    BlockFooter *footer = find_block_footer(header);

    // pointer manipulation to remove the block from the free list

    // check if the block is the head of the free list
    if (free_list == footer)
    {
        free_list = footer->free_list_next;
    }

    // check if the block is the tail of the free list
    if (footer->free_list_next != NULL)
    {
        footer->free_list_next->free_list_prev = footer->free_list_prev;
    }

    // check if the block is the head of the free list
    if (footer->free_list_prev != NULL)
    {
        footer->free_list_prev->free_list_next = footer->free_list_next;
    }

    // clear the pointers in the footer for good measure
    footer->free_list_next = NULL;
    footer->free_list_prev = NULL;
}

/*
 * Add the given block to the free list.
 *
 * Parameters:
 * - header: the header of a free block to add to the free list.
 */
static void add_to_free_list(BlockHeader *header)
{
    BlockFooter *footer = find_block_footer(header);

    // insert block at the head of the free list
    // need to link this block with what is currently at the first position in the free list, if it exists

    // set next pointer of this block's footer to the current head of the free list
    footer->free_list_next = free_list;

    // if not empty, set prev pointer of the current first plock to this
    if (free_list != NULL)
    {
        free_list->free_list_prev = footer;
    }

    // update free list pointer to this block, making it the new head
    free_list = footer;
}

/*
 * Mark this block as used. Update its header. Update the header of its
 * neighbor.
 *
 * In PART B, this should also remove the block from the free list.
 *
 * Parameters:
 * - header: the header of the block we wish to mark as used.
 */
static void make_block_used(BlockHeader *header)
{
    // set A bit to used
    set_block_abit(header, ABIT_USED);

    // if the neighbor is not the end mark, set its p-bit
    BlockHeader *next_block = get_next_adjacent_block(header);
    if (!is_end_mark(next_block))
    {
        set_block_pbit(next_block, PBIT_USED);
    }

    remove_from_free_list(header);
}

/*
 * Mark this block as free. Update its header AND footer. Update the header of
 * its neighbor.
 *
 * In PART B, this should also add the block to the free list.
 *
 * Parameters:
 * - header: the header of the block we wish to mark as free.
 */
static void make_block_free(BlockHeader *header)
{
    // clear A bit to free
    set_block_abit(header, ABIT_FREE);

    // create or update block's footer, since it is now free.
    BlockFooter *footer = find_block_footer(header);
    set_block_footer_size(footer, get_block_size(header));

    // get neighbor block and clear its p-bit
    BlockHeader *next_block = get_next_adjacent_block(header);
    if (!is_end_mark(next_block))
    {
        set_block_pbit(next_block, PBIT_FREE);
    }

    // finally, add the block to the free list since it is now free
    add_to_free_list(header);
}

/*
 * Determines whether the given block is large enough to split into the desire
 * size and a whole other block. Recall that the minimum size of a block is
 * MIN_BLOCK_SIZE.
 *
 * Does not actually split the block -- just determines if it can be split.
 *
 * Parameters:
 * - header: the original block that we may want to split
 * - desired_size: the size of the block we wish to allocate.
 *
 * Returns:
 * - a true value if and only if this block is large enough to allocate
 * desired_size and still have enough space for an entire other block with its
 * header.
 */
static int should_split(BlockHeader *header, size_t desired_size)
{
    // we simply need to compare the size of the block to the desired size plus the minimum block size
    return get_block_size(header) >= desired_size + MIN_BLOCK_SIZE;
}

/*
 * Split the given block if it can be split. If it can be split, the original
 * block should be split into a new block of the desired_size. The remaining
 * space should be made into a new block with a valid block header.
 *
 * If the block cannot be split, then nothing happens and we return null.
 *
 * NOTE: (PART B) Be careful around how you handle free lists so that you don't
 * corrupt the linked list!
 *
 * Parameters:
 * - header: the original block that we want to split if possible.
 * - desired_size: the size of the block we wish to allocate.
 *
 * Return:
 * - NULL if the block was not split
 * - a pointer to the header of the newly created remainder block if the block
 *   was split.
 */
static BlockHeader *split_block(BlockHeader *header, size_t desired_size)
{

    if (!should_split(header, desired_size))
    {
        return NULL;
    }

    // update the size of the original block
    size_t remainder_size = get_block_size(header) - desired_size - sizeof(BlockHeader);
    set_block_header(header, desired_size, ABIT_USED, PBIT_USED);

    // update the remainder block's header
    BlockHeader *remainder_block = get_next_adjacent_block(header);
    set_block_header(remainder_block, remainder_size, ABIT_FREE, PBIT_USED);

    // update the footer of the remainder block
    BlockFooter *remainder_footer = find_block_footer(remainder_block);
    set_block_footer_size(remainder_footer, remainder_size);

    // update the P bit of the next block
    BlockHeader *next_block = get_next_adjacent_block(remainder_block);
    if (next_block && !is_end_mark(next_block))
    {
        set_block_pbit(next_block, PBIT_FREE);
    }

    // return new block's header to caller
    return remainder_block;
}

/*
 * Coalesce this block with the block after it.
 *
 * NOTE: Be careful around how you handle free lists so that you don't
 * corrupt the linked list!
 *
 * Parameters:
 * - header: the block that comes first in the heap spatially (not first in the
 *   free list).
 */
static void coalesce_with_next_block(BlockHeader *header)
{

    // retrieve the next block
    BlockHeader *next_block = get_next_adjacent_block(header);

    // remove the first block from the free list
    remove_from_free_list(header);

    // update the size of the block
    size_t new_size = get_block_size(header) + get_block_size(next_block) + sizeof(BlockHeader);

    BlockFooter *next_footer = find_block_footer(next_block);
    set_block_footer_size(next_footer, new_size);

    set_block_header(header, new_size, ABIT_FREE, get_block_pbit(header));
}

///////////////////////////////////////////////////////////////////////////////
// The actual heap implementation
///////////////////////////////////////////////////////////////////////////////

/*
 * Allocate 'size' bytes of heap memory.
 *
 * This function must:
 * - Return NULL if size < 1
 * - Determine block size. If the requested size is smaller than the min block
 *   size, increase it to the min block size. Round up to a multiple of
 *   BLOCK_ALIGNMENT and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If there is no large-enough available block, return NULL.
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements
 *       - Update all heap block header(s) and footer(s)
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * In Part B, this should also remove the given block from the free list.
 *
 * Parameters:
 * - size: the requested payload size in bytes
 *
 * Returns:
 * - the address of the allocated memory (the payload, not the header!) on
 *   sucessful allocation
 * - NULL on failure
 *
 */
void *balloc(size_t size)
{
    if (size < 1)
    {
        return NULL;
    }

    // round up the size
    size_t rounded_size = round_up_block_size(size);
    if (rounded_size < MIN_BLOCK_SIZE)
    {
        rounded_size = MIN_BLOCK_SIZE;
    }

    // find the best fit block
    BlockHeader *best_fit = best_fit_select_block(rounded_size);

    // check if there is a best fit block
    if (!best_fit)
    {
        // no best fit block found
        return NULL;
    }

    // check if the best fit block is an exact match. if not, split the block
    if (should_split(best_fit, rounded_size))
    {
        // split the block
        split_block(best_fit, rounded_size);
    }

    // mark the block as used
    make_block_used(best_fit);

    // return the address of the allocated memory
    return get_block_payload(best_fit);
}

/*
 * Free a previously allocated block.
 *
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 * - Coalesce the free block with it's adjacent neighbors on both sides as much
 *   as possible.
 *
 * In part B, this should also add the block to the head of the free list.
 *
 * Parameters:
 * - ptr: the address of a previously allocated block to be freed.
 *   Note that this is the address of the payload, not the header!
 *
 * Returns:
 * - 0 on success
 * - -1 on failure
 *
 */
int bfree(void *ptr)
{
    BlockHeader *header = (BlockHeader *)(((char *)ptr) - sizeof(BlockHeader));

    // check if ptr is NULL or not a multiple of 8
    if (header == NULL || ((unsigned int)ptr % 8 != 0))
    {
        return -1;
    }

    // check if ptr is outside of the heap space.
    if ((char *)header > ((char *)heap_start + heap_size) || (char *)header < (char *)heap_start)
    {
        return -1;
    }

    // check if the block is already freed
    if (!get_block_abit((BlockHeader *)header))
    {
        return -1; // cannot free a freed block!!
    }

    // make the block free
    make_block_free(header);

    // initiate coalescing if possible
    BlockHeader *curr = header;
    int continue_coalescing = 1; // flag to continue the loop

    while (continue_coalescing) // loop until no more coalescing can be done
    {
        continue_coalescing = 0; // reset flag at loop start

        if (get_block_pbit(curr) == PBIT_FREE && get_prev_adjacent_block(curr) >= heap_start)
        {
            curr = get_prev_adjacent_block(curr);
            coalesce_with_next_block(curr);
            continue_coalescing = 1; // continue if coalescing happened
        }
        else
        {
            BlockHeader *next = get_next_adjacent_block(curr);
            if (next && !is_end_mark(next) && get_block_abit(next) == ABIT_FREE)
            {
                coalesce_with_next_block(curr);
                continue_coalescing = 1; // continue if coalescing happened
            }
        }
    }

    return 0;
}

/*
 * Initialize our memory allocator.
 *
 * This should be called EXACTLY ONCE by a program before any other calls to the
 * heap implementation.
 *
 * Parameters:
 * - size: the size of the heap space to be allocated in bytes. Must be
 *   a multiple of 4096.
 *
 * Returns:
 * - 0 on success
 * - -1 on failure
 */
int init_heap(size_t size)
{

    // Prevent multiple calls to initialize the heap
    static int allocated_once = 0;
    if (allocated_once != 0)
    {
        fprintf(stderr,
                "Error: init_heap has allocated space during a previous call\n");
        return -1;
    }

    // Get the pagesize from the OS... this is almost certainly 4096 bytes (4KB).
    int pagesize = getpagesize();

    if ((size < 1) || ((size % pagesize) != 0))
    {
        fprintf(stderr, "Error: Requested block size is not a positive"
                        " multiple of page size.\n");
        return -1;
    }

    // Using mmap to allocate memory + enough space for guard pages.
    // (see man mmap if you are curious)
    void *mmap_ptr = mmap(NULL, size + 2 * pagesize,
                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == mmap_ptr)
    {
        fprintf(stderr, "Error: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }

    allocated_once = 1;
    heap_size = size;

    // Create a guard page at either end of the heap, and set their permissions
    // to PROT_NONE. This means that if we accidentally write past either end of
    // the heap, we will get a segfault, rather than a silent memory corruption.
    // (see man mprotect if you are curious)
    void *start_guard_page = mmap_ptr;
    void *end_guard_page = ((char *)mmap_ptr) + pagesize + size;

    mprotect(start_guard_page, pagesize, PROT_NONE);
    mprotect(end_guard_page, pagesize, PROT_NONE);

    // Create an "end mark" -- a block at the end of the heap that is never
    // free. This reduces the number of special cases we need to deal with.
    BlockHeader *end_mark = (BlockHeader *)((char *)end_guard_page - MIN_BLOCK_SIZE);
    end_mark->size_status = END_MARK_MAGIC;

    // Initially there is only one big free block in the heap.
    //
    // When computing the size, we need to account for the end mark and the
    // block header.
    //
    // Set p-bit as allocated in header to avoid trying to coalesce it with
    // invalid memory before the heap. Note the a-bit is set as free.
    heap_start = (BlockHeader *)(((char *)start_guard_page) + pagesize);

    // DO NOT CHANGE ANYTHING ABOVE THIS LINE
    set_block_header(heap_start,
                     heap_size - MIN_BLOCK_SIZE - sizeof(BlockHeader),
                     ABIT_FREE, PBIT_USED);

    // Make the block free. This takes care of the footer and free list too.
    make_block_free(heap_start);

    return 0;
}

/*
 * Function can be used for DEBUGGING to help you visualize your heap structure.
 * Traverses heap blocks and prints info about each block found.
 *
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * Begin  : address of the first byte in the block (where the header starts)
 * End    : address of the last byte in the block
 * Size   : size of the block as stored in the block header
 */
void disp_heap()
{
    BlockHeader *current = heap_start;
    BlockHeader *next_block;

    size_t counter = 1;
    size_t used_size = 0;
    size_t free_size = 0;
    int is_used = -1;
    int is_p_used = -1;
    size_t size = 0;

    fprintf(stdout, "Heap Start: %p\n", heap_start);
    fprintf(stdout, "Heap Size: %d\n", heap_size);

    fprintf(stdout,
            "*********************************** HEAP: Block List ****************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tBegin\t\tEnd\t\tSize\n");
    fprintf(stdout,
            "---------------------------------------------------------------------------------\n");

    while (!is_end_mark(current))
    {
        is_used = get_block_abit(current) == ABIT_USED;
        is_p_used = get_block_pbit(current) == PBIT_USED;
        next_block = get_next_adjacent_block(current);
        size = get_block_size(current);

        if (is_used)
            used_size += size;
        else
            free_size += size;

        fprintf(stdout, "%u\t%s\t%s\t0x%08lx\t0x%08lx\t%4u\n",
                counter, is_used ? "alloc" : "FREE",
                is_p_used ? "alloc" : "FREE",
                (unsigned long)current,
                (unsigned long)next_block,
                get_block_size(current));

        current = next_block;
        counter += 1;
    }

    fprintf(stdout,
            "---------------------------------------------------------------------------------\n");
    fprintf(stdout,
            "*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4u\n", used_size);
    fprintf(stdout, "Total free size = %4u\n", free_size);
    fprintf(stdout, "Total size      = %4u\n", used_size + free_size);
    fprintf(stdout,
            "*********************************************************************************\n");
    fflush(stdout);

    return;
}