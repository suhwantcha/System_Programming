/*
 * This implementation uses a segregated list allocator with 20 size classes.
 * It employs a Best-Fit allocation strategy by maintaining size-ordered free lists
 * for each class, which reduces internal fragmentation.
 * Blocks are split to minimize fragmentation, and coalescing uses boundary tags.
 * Realloc is optimized to reduce data copying by coalescing with adjacent blocks
 * or extending the heap directly if possible.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
        /* Your student ID */
        "20222089",
        /* Your full name*/
        "SUHWAN TCHA",
        /* Your email address */
        "dndoehrk11@gmail.com",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4          /* Word and header/footer size (bytes) */
#define DSIZE 8          /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define SEG_LIST_COUNT 20 /* Number of segregated lists */
#define MIN_BLOCK_SIZE 16 /* Minimum block size */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLK(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLK(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Pointers to previous and next free blocks in the same size class list */
#define PREV_FREE(bp) (*(void **)(bp))
#define NEXT_FREE(bp) (*(void **)((char *)(bp) + WSIZE))

/* Global variables */
static void **free_lists;

/* Helper function prototypes */
static int get_class_index(size_t size);
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void insert_list(void *bp);
static void delete_list(void *bp);

/*
 * get_class_index - Given a size, returns the appropriate size class index.
 */
static int get_class_index(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    if (size <= 4096) return 8;
    if (size <= 8192) return 9;
    if (size <= 16384) return 10;
    if (size <= 32768) return 11;
    if (size <= 65536) return 12;
    if (size <= 131072) return 13;
    if (size <= 262144) return 14;
    if (size <= 524288) return 15;
    if (size <= 1048576) return 16;
    if (size <= 2097152) return 17;
    if (size <= 4194304) return 18;
    return 19;
}

/*
 * mm_init - Initialize the malloc package.
 */
int mm_init(void) {
    char *heap_prologue;
    /* Allocate space for segregated list heads on the heap */

    if ((free_lists = mem_sbrk(SEG_LIST_COUNT * sizeof(void *))) == (void *)-1)
        return -1;

    /* Initialize all list heads to NULL */
    for (int i = 0; i < SEG_LIST_COUNT; i++) {
        free_lists[i] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_prologue = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_prologue, 0);                            /* Alignment padding */
    PUT(heap_prologue + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_prologue + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_prologue + (3 * WSIZE), PACK(0, 1));   /* Epilogue header */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block with Best-Fit strategy.
 */
void *mm_malloc(size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void *bp = NULL;

    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = MIN_BLOCK_SIZE;
    else
        asize = ALIGN(size + DSIZE);

    /* Search the free list for a fit */
    int class_idx = get_class_index(asize);
    for (int i = class_idx; i < SEG_LIST_COUNT; i++) {
        void *current = free_lists[i];
        while (current != NULL) {
            if (GET_SIZE(HDRP(current)) >= asize) {
                bp = current;
                goto found;
            }
            current = NEXT_FREE(current);
        }
    }

    found:
    /* No fit found. Get more memory and place the block */
    if (bp == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }

    bp = place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block.
 */
void mm_free(void *bp) {
    if (bp == NULL)
        return;

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block.
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLK(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        insert_list(bp);
        return bp;
    } else if (prev_alloc && !next_alloc) {
        delete_list(NEXT_BLK(bp));
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        delete_list(PREV_BLK(bp));
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        bp = PREV_BLK(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else {
        delete_list(PREV_BLK(bp));
        delete_list(NEXT_BLK(bp));
        size += GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(FTRP(NEXT_BLK(bp)));
        bp = PREV_BLK(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    insert_list(bp);
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size = ALIGN(size + DSIZE);
    if (new_size < MIN_BLOCK_SIZE) new_size = MIN_BLOCK_SIZE;

    /* If new size is smaller or equal, we can often return the same block */
    if (old_size >= new_size) {
        if (old_size - new_size >= MIN_BLOCK_SIZE) {
            place(ptr, new_size); // Split the block
        }
        return ptr;
    }

    /* Try to coalesce with the next block */
    void *next_block = NEXT_BLK(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(next_block));
    size_t next_size = GET_SIZE(HDRP(next_block));

    if (!next_alloc && (old_size + next_size >= new_size)) {
        delete_list(next_block);
        size_t total_size = old_size + next_size;
        PUT(HDRP(ptr), PACK(total_size, 1));
        PUT(FTRP(ptr), PACK(total_size, 1));
        return ptr;
    }

    /* If the block is the last one, extend the heap */
    if (!next_alloc && next_size == 0) { // next block is the epilogue
        size_t extend_size = new_size - old_size;
        if(mem_sbrk(extend_size) == (void *)-1) return NULL;
        PUT(HDRP(ptr), PACK(new_size, 1));
        PUT(FTRP(ptr), PACK(new_size, 1));
        PUT(HDRP(NEXT_BLK(ptr)), PACK(0,1));
        return ptr;
    }

    /* Allocate a new block as a last resort */
    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    memcpy(new_ptr, ptr, old_size - DSIZE);
    mm_free(ptr);
    return new_ptr;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t size) {
    char *bp;
    size_t asize = ALIGN(size);

    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(asize, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(asize, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLK(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * insert_list - Insert a free block into the correct size-ordered list.
 * This enables Best-Fit.
 */
static void insert_list(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int idx = get_class_index(size);
    void *current = free_lists[idx];
    void *prev = NULL;

    // Find insertion point to keep the list sorted by size
    while (current != NULL && GET_SIZE(HDRP(current)) < size) {
        prev = current;
        current = NEXT_FREE(current);
    }

    if (prev == NULL) { // Insert at the beginning of the list
        NEXT_FREE(bp) = current;
        if (current != NULL) {
            PREV_FREE(current) = bp;
        }
        free_lists[idx] = bp;
        PREV_FREE(bp) = NULL;
    } else { // Insert in the middle or at the end
        NEXT_FREE(bp) = current;
        if (current != NULL) {
            PREV_FREE(current) = bp;
        }
        NEXT_FREE(prev) = bp;
        PREV_FREE(bp) = prev;
    }
}

/*
 * delete_list - Remove a block from its free list.
 */
static void delete_list(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int idx = get_class_index(size);

    void *prev = PREV_FREE(bp);
    void *next = NEXT_FREE(bp);

    if (prev == NULL) { // Block is at the head of the list
        free_lists[idx] = next;
    } else {
        NEXT_FREE(prev) = next;
    }

    if (next != NULL) {
        PREV_FREE(next) = prev;
    }
}

/*
 * place - Place block of asize bytes at start of free block bp
 * and split if remainder would be at least minimum block size.
 * Applies a heuristic for splitting.
 */
static void *place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remainder = csize - asize;

    delete_list(bp);

    if (remainder < MIN_BLOCK_SIZE) {
        /* Do not split, use the entire block */
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        return bp;
    }
        // For smaller allocations, place the free block first.
        // This can improve utilization for certain workloads by keeping smaller address spaces free.
    else if (asize < 100) {
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        void* alloc_bp = NEXT_BLK(bp);
        PUT(HDRP(alloc_bp), PACK(asize, 1));
        PUT(FTRP(alloc_bp), PACK(asize, 1));
        insert_list(bp);
        return alloc_bp;
    }
    else {
        /* Place allocated block first */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *new_bp = NEXT_BLK(bp);
        PUT(HDRP(new_bp), PACK(remainder, 0));
        PUT(FTRP(new_bp), PACK(remainder, 0));
        insert_list(new_bp);
        return bp;
    }
}