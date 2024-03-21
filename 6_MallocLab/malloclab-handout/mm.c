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
 * with the area itself storing data (size_t) 0 & 0x1.
 *
 * The first call to mm_malloc(size) expands the heap by calling
 * mem_sbrk(ALIGN(size + SIZE_T_SIZE)). Note that `size` refers to the
 * payload size.
 *
 * |<--SIZE_T_SIZE-->|<-----------ALIGN(size + SIZE_T_SIZE)----------->|
 * |padding|---0/1---|<-------`size`------->|padding?|padding|---?/?---|
 * |**(A)**|***(B)***|*************(C)***************|**(A')*|***(B')**|
 * :The size of additional padding (A') is 8 - `size` % 8, to ensure the block
 * is aligned to 8 bytes.
 *
 * The payload uses area (C)+(A'), and area (B) serves as the block's header.
 * the block size, still ALIGN(size + SIZE_T_SIZE), is stored in area (B)
 * Finally, the area (B') is marked as tail block (0/1), signaling the end
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

/* My helper routines */
static void *target_block(int blocksize);
static int is_allocated(size_t header);
static int set_allocated(size_t *p);

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

/* sizeof(size_t) = 4, ALIGN(4) = 8 */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 8 - 4 */
#define SIZE_T_PADDING SIZE_T_SIZE - sizeof(size_t)

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *p;
    if ((p = mem_sbrk(SIZE_T_SIZE)) == (void *)-1)
        return -1;
    p = (void *)((char *)p + SIZE_T_PADDING); /* Set p as pointer to header */
    *(size_t *)p = 0;
    set_allocated(p);
    
    return 0;
}

/* 
 * mm_malloc - Allocate appropriate free block by tranversing
 *        implicit free list.
 */
void *mm_malloc(size_t size)
{
    const int blksize = ALIGN(size + SIZE_T_SIZE); /* Block size to malloc */
    void *p;                                   /* Pointer to malloced block */
    
    /* Traverse the free list. */
    if ((p = target_block(blksize)) != NULL)
        return (void *)((char *)p + sizeof(size_t));
    
    /* There is no appropriate free block on the heap. */
    p = mem_sbrk(blksize);
    if (p == (void *)-1) {
	return NULL;
    } else {
        /* Store block size to header */
        size_t *hp = (size_t *)((char *)p - sizeof(size_t));
        *hp = blksize;
        set_allocated(hp);

        /* Set header of last block as 0/1 */
        size_t *lhp = (size_t *)((char *)hp + blksize);
        *lhp = 0;
        set_allocated(lhp);
        
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
        if (!is_allocated(*p) && size >= blocksize && size < min) {
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
 * is_allocated - Check if LSB of header is 1
 */
static int is_allocated(size_t header)
{
    return header & 0x1;
}

static int set_allocated(size_t *p)
{
    if (p == NULL)
        return -1;
    *p |= 0x1;                  /* set LSB to 1 */
    return 0;
}

/*****************************
 * End of My helper routines *
 *****************************/
