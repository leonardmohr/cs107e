/*
 * File: malloc.c
 * --------------
 * This is the simple "bump" allocator from lecture.
 * An allocation request is serviced by tacking on the requested
 * space to the end of the heap thus far. 
 * It does not recycle memory (free is a no-op) so when all the
 * space set aside for the heap is consumed, it will not be able
 * to service any further requests.
 *
 * This code is given here just to show the very simplest of
 * approaches to dynamic allocation. You are to replace this code
 * with your own heap allocator implementation.
 */

#include "malloc.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "printf.h"

extern int __bss_end__;

// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two -- why?
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

#define TOTAL_HEAP_SIZE 0x1000000 // 16 MB


/* Global variables for the bump allocator
 *
 * `heap_start` tracks where heap segment begins in memory.
 * It is initialized to point to the address at end of data segment.
 * It uses symbol __bss_end__ from memmap to locate end.
 * `heap_used` tracks the number of bytes allocated thus far.
 * The next available memory begins at heap_start + heap_used.
 * `heap_max` is total number of bytes set aside for heap segment.
 */
static void *heap_start = NULL;
static int heap_max = TOTAL_HEAP_SIZE;

typedef struct header header;
// status is 0 if free, 1 if in use
struct header {
    size_t payload_size;
    int status;
};
const unsigned int header_size = sizeof(header);

void coalesce(header *cur_hdr);

void *malloc(size_t nbytes) 
{
    if (nbytes < 1) return NULL;

    // header for start of whole heap block
    header *init_hdr = 0;

    if (!heap_start) {
        heap_start = &__bss_end__;
        init_hdr = (header *) heap_start;
        init_hdr->payload_size = heap_max - header_size;
        init_hdr->status = 0;
    }

    nbytes = roundup(nbytes, 8);

    void *alloc = NULL;
    header *cur_hdr = heap_start;
    int found_free = 0;
    int make_new_hdr = 0;

    while ((char *) cur_hdr < (char *) heap_start + heap_max && !found_free) {
        if (cur_hdr->status == 1 || cur_hdr->payload_size < nbytes) {
            cur_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
        } else {
            found_free = 1;
            if (cur_hdr->payload_size >= nbytes + header_size) {
                make_new_hdr = 1;
            }
        }
    }

    if (found_free) {
        // check if there's enough space in heap
        if ((char *) cur_hdr + header_size + nbytes > (char *) heap_start + heap_max)
            return NULL;

        // update the block header to be in use and
        // the number of bytes
        unsigned int old_blk_size = cur_hdr->payload_size;
        cur_hdr->payload_size = nbytes;
        cur_hdr->status = 1;

        alloc = (char *) cur_hdr + header_size;

        // create a new header for what's
        // left of the free block
        if (make_new_hdr) {
            header *new_hdr = (header *) ((char *) alloc + nbytes);
            new_hdr->payload_size = old_blk_size - nbytes - header_size;
            new_hdr->status = 0;
            coalesce(new_hdr);
        }
    }

    return alloc;
}

// helper function for coalescing blocks
void coalesce(header *cur_hdr)
{
    header *next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
    while ((char *) cur_hdr < (char *) heap_start + heap_max && next_hdr->status == 0) {
        cur_hdr->payload_size += next_hdr->payload_size + header_size;
        next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
    }
}

void free(void *ptr) 
{
    if (ptr == NULL) return;

    header *cur_hdr = (header *) ((char *) ptr - header_size);
    cur_hdr->status = 0;

    // coalesce any following free blocks
    coalesce(cur_hdr);
}

void *realloc(void *old_ptr, size_t new_size)
{
    // edge cases for null ptr and resize of 0
    if (old_ptr == NULL) return NULL;
    if (new_size == 0) {
        free(old_ptr);
        return NULL;
    }

    header *cur_hdr = (header *) ((char *) old_ptr - header_size);
    int old_size = cur_hdr->payload_size;
    new_size = roundup(new_size, 8);

    // if the new size is the same as the old size
    // do nothing
    if (old_size == new_size) return old_ptr;

    void *new_ptr = NULL;
    // if the current block is already big enough
    // split off a block to be free
    if (old_size > new_size) {
        cur_hdr->payload_size = new_size;
        header *next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
        next_hdr->payload_size = old_size - new_size - header_size;
        next_hdr->status = 0;
        coalesce(next_hdr);
        new_ptr = (char *) cur_hdr + header_size;
        return new_ptr;
    // if the block doesn't have enough space
    } else {
        // check if the next block is free and
        // has space if coalesced with the current block
/*You can just call coalesce here if old_blck is not big enough. If fact, you can call coalesce regardless of whether old_block is big enough.
    header *next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
    while ((char *) cur_hdr < (char *) heap_start + heap_max && next_hdr->status == 0) {
        cur_hdr->payload_size += next_hdr->payload_size + header_size;
        next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
    }
*/
        header *next_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
        if (next_hdr->status == 0 && (old_size + next_hdr->payload_size + header_size >= new_size)) {
            cur_hdr->payload_size = new_size;
            header *new_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
            new_hdr->payload_size = next_hdr->payload_size - (new_size - old_size);
            new_hdr->status = 0;
            new_ptr = (char *) cur_hdr + header_size;
            return new_ptr;
        }
        // worst case, find a new block
        void *new_ptr = malloc(new_size);
        if (!new_ptr) return NULL;
        memcpy(new_ptr, old_ptr, new_size);
        free(old_ptr);
        return new_ptr;
    }
    return new_ptr;
}

void heap_dump () {
    printf("Starting heap dump:\n");
    header *cur_hdr = (header *) heap_start;
    int i = 0;
    while ((char *) cur_hdr < (char *) heap_start + heap_max) {
        int payload = cur_hdr->payload_size;
        int status = cur_hdr->status;
        printf("%d: Header at %p, payload of %d, status of %d\n", i, (char *) cur_hdr, payload, status);

        cur_hdr = (header *) ((char *) cur_hdr + cur_hdr->payload_size + header_size);
        i++;
    }
}
