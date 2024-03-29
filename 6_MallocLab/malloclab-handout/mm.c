/*
 * mm.c - memory allocation module using implicit free list.
 *
 * Slow, but good memory efficiancy.
 * ----
 * NOTE: This code is for 32-bit system; Compile it with -m32 flag.
 * ----(ALIGNMENT)----
 * mm_init() sets heap like this:
 *
 * 0                      4                      8
 * |<----------------SIZE_T_SIZE---------------->|
 * |<---SIZE_T_PADDING--->|<---sizeof(size_t)--->|
 * |(This area is padding)|(This area is header) |
 * |----------------------|----------0/1---------|
 * :"0/1" indicates that the block is size 0, and is alloacted(1)
 * with the area itself storing a (size_t) value 0 & 0x1.
 *
 * The first call to mm_malloc(`size`) expands the heap by calling
 * mem_sbrk(ALIGN(size + sizeof(size_t))). Note that `size`
 * refers to the required payload size. We need to allocate
 * more space than that and serve to caller.
 *
 * |<--SIZE_T_SIZE-->|<-----ALIGN(size + sizeof(size_t))----->|
 * |padding|---0/1---|<-------`size`------->|padding|---?/?---|
 * |**(A)**|***(B)***|**********(C)*********|**(A')*|***(B')**|
 * |///////|----------------(block)-----------------|--(tail)-|
 *
 * The payload uses area (C)+(A'), and area (B) serves as the block's header.
 * The block size, still ALIGN(size + sizeof(size_t)), since (B) and (B') are
 * both same size (sizeof(size_t)), is stored in area (B). Finally,
 * the area (B') is marked as tail block (0/1), signaling the end
 * of the free list.
 *
 * As we marked (B') as 0/1, each subsequent increase in the heap size follows
 * the same pattern, recursively aligning each block by marking the header
 * and the last block.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static void *target_block(int blocksize);
static int set_header(size_t *header, size_t blocksize, int flag);

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "alpha",
    /* First member's full name */
    "Dillution",
    /* First member's email address */
    "hskimse1@gmail.com",
    "",
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define SIZE_T_PADDING SIZE_T_SIZE - sizeof(size_t)

#define IS_ALLOCATED(header) ((header) & 0x1)

/* flags for function set_header */
#define HDR_FREE 0
#define HDR_ALLOCATED 1

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* allocate initial block */
    void *p;
    if ((p = mem_sbrk(SIZE_T_SIZE)) == (void *)-1)
        return -1;
    p = (void *)((char *)p + SIZE_T_PADDING); /* Set p as pointer to header */
    set_header(p, 0, HDR_ALLOCATED);
    return 0;
}

/* 
 * mm_malloc - Allocate appropriate free block by tranversing
 *        implicit free list.
 */
void *mm_malloc(size_t size)
{
    void *p;
    const int blksize = ALIGN(size + sizeof(size_t)); /* Block size to malloc */
    
    /* Traverse the free list. */
    if ((p = target_block(blksize)) != NULL) /* pointer to new block's header */
        return (void *)((char *)p + sizeof(size_t)); /* return pointer to
                                                        payload */
    /* There is no appropriate free block on the heap. */
    p = mem_sbrk(blksize);      /* pointer to new block's payload */
    if (p == (void *)-1) {
        return NULL;
    } else {
        /* Store block size to header */
        size_t *hp = (size_t *)((char *)p - sizeof(size_t));
        set_header(hp, blksize, HDR_ALLOCATED);

        /* Set header of last block as 0/1 */
        size_t *lhp = (size_t *)((char *)hp + blksize);
        set_header(lhp, 0, HDR_ALLOCATED);

        return (void *) p;      /* return pointer to payload */
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************
 * My helper routines *
 **********************/

/*
 * target_block - return the pointer to head of appropriate free block while
 *       traverse the implicit list. return NULL if there is no space.
 */
static void *target_block(int blocksize)
{
    char *p = (char *) mem_heap_lo() + SIZE_T_PADDING; /* Pointer to each
                                                        * block's header */
    size_t size = *(size_t *)p & ~0x7;   /* Blocksize of current iteration. */
    void *dest = NULL;                   /* Target block. */
    size_t min = -1U;              /* Minimum blocksize for entire iteration */

    while (size != 0) {
        /* If p is the appropriate block, load it to dest */
        if (!IS_ALLOCATED(*p) && size >= blocksize && size < min) {
            dest = p;
            min = size;
        }
        
        /* Traverse to the next block */
        p += size;
        size = *(size_t *)p & ~0x7;
    }
    return dest;
}

/*
 * set_header - set block's header to blocksize marked with flag.
 *      if blocksize is negative, just mark to header's element.
 *      flag should be only HDR_FREE or HDR_ALLOCATED.
 *      return -1 if header is NULL, 0 if no error.
 */
static int set_header(size_t *header, size_t blocksize, int flag)
{
    if (header == NULL)
        return -1;
    if (blocksize < 0)
        *header |= flag;
    else
        *header = blocksize | flag;
    return 0;
}

/*****************************
 * End of My helper routines *
 *****************************/
