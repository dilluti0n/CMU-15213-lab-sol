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

/* NOW : Implement mm_free()
   TODO: Documnetation, Improve readability for macros,
        Increase abstraction level of macros related to HDR_ flags */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static int block_size;

static void *get_target_block(size_t blocksize);
static int set_header(void *header, size_t blocksize, int flag);
static int set_footer(void *header, size_t blocksize);

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

/* Double word alignment */
#define DWRD_ALIGN

/* Alignment */
#ifdef SWRD_ALIGN
#define ALIGNMENT 4
#define HDR_MASK 0x3
#endif

#ifdef DWRD_ALIGN
#define ALIGNMENT 8
#define HDR_MASK 0x7
#endif

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define SIZE_T_PADDING SIZE_T_SIZE - sizeof(size_t)

#define IS_ALLOC(header) ((header)&HDR_ALLOC)
#define IS_PFREE(header) ((header)&HDR_PFREE)
 
/* flags for function set_header */
#define HDR_FREE 0
#define HDR_ALLOC 1         /* This block is allocated */
#define HDR_PFREE 2           /* Previous block is free */

#define DEBUG
#ifdef DEBUG
#define DBG_CHECK printf("[%3i]: %s\n",__LINE__, __func__);
#else
#define DBG_CHECK
#endif

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
    set_header(p, 0, HDR_ALLOC);
    DBG_CHECK
    return 0;
}

/* 
 * mm_malloc - Allocate appropriate free block by tranversing
 *        implicit free list.
 */
void *mm_malloc(size_t size)
{
    void *p;
    const size_t blksize = ALIGN(size + sizeof(size_t)); /* Block size to malloc */
    
    /* Traverse the free list. */
    if ((p = get_target_block(blksize)) != NULL) {
        const size_t tmp = block_size - blksize;       /* Size of empty space */
        
        /* Mark target block for use */
        set_header(p, blksize, HDR_ALLOC);
        
        /* Mark empty space as free if there is empty space */
        void *next = (void *)((char *)p + blksize);
        if (tmp != 0) {
            set_header(next, (size_t) tmp, HDR_FREE);
            set_footer(next, (size_t) tmp);
        } else {                /* There is no empty space */
            /* Mark next block as "Previouse alloc" */
            *(size_t *)next = *(size_t *)next & ~HDR_PFREE;
        }

        DBG_CHECK
        /* Return pointer to payload */
        return (void *)((char *)p + sizeof(size_t));
    }

    /* There is no appropriate free block on the heap. */
    p = (void *)((char *)mem_heap_hi() - sizeof(size_t) + 1);
    size_t tmp = *(size_t *)p;           /* Header element of last block */
    size_t excess = blksize;             /* Requirement of heap */
    if (IS_PFREE(tmp)) {
        size_t psize = *((size_t *)p - 1) & ~HDR_MASK; /* Previous block's size */
        excess -= psize;
        p = (void *)((char *)p - psize); /* Our block should start at here. */
    }
    if (mem_sbrk(excess) == (void *)-1) {
        return NULL;
    }
    /* Store block size to header */
    set_header(p, blksize, HDR_ALLOC);

    /* Set header of last block as 0/01 */
    set_header((void *)((char *)p + blksize), 0, HDR_ALLOC);

    DBG_CHECK
    return (void *)((size_t *)p + 1);    /* return pointer to payload */
}

/*
 * mm_free - Freeing a block by passing argument pointer to payload.
 */
void mm_free(void *ptr)
{
    char *p = (char *)ptr - sizeof(size_t);  /* Pointer to header */
    const size_t hdr = *(size_t *)p;         /* Header element */
    size_t blksize = hdr & ~HDR_MASK;        /* Blocksize */
    size_t flag = HDR_FREE;                  /* Flag for freeing block */

    /* Merge with next block */
    size_t tmp = *(size_t *)(p + blksize);   /* Header element of next block */
    if (!IS_ALLOC(tmp))
        blksize += tmp & ~HDR_MASK;

    /* Merge with previous block */
    if (IS_PFREE(hdr)) {
        /* Header element of previous block */
        tmp = *(size_t *)(p - sizeof(size_t));

        /* Set flag and blocksize and header */
        const size_t pbsize = tmp & ~HDR_MASK; /* Size of previous block */
        flag = tmp & HDR_MASK;
        blksize += pbsize;
        p -= pbsize;            /* Mearge two block */
    }
    
    /* Set this block's marking */
    set_header((void *)p, blksize, flag);
    set_footer((void *)p, blksize);

    /* Set next block's marking */
    char *np = p + blksize;                /* Pointer to next block's header */
    tmp = *(size_t *)np & ~HDR_MASK;            /* Next block's size */
    set_header((size_t *)np, tmp, HDR_PFREE | HDR_ALLOC);
    DBG_CHECK
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
 * get_target_block - return the pointer to head of appropriate free block while
 *       traversing the implicit list. return NULL if there is no space.
 *
 *       Set global variable islastalloc as 1 if last iteration of searching
 *       block is allocated, 0 if free.
 */
static void *get_target_block(size_t blocksize)
{
    char *p = (char *)mem_heap_lo() + SIZE_T_PADDING;
                                              /* Pointer to each block's header */
    size_t size = *(size_t *)p & ~HDR_MASK;   /* Blocksize of current iteration. */
    void *dest = NULL;                        /* Target block. */
    size_t min = -1U;              /* Minimum blocksize for entire iteration */

    while (size != 0) {
        /* If p is the appropriate block, load it to dest */
        if (!IS_ALLOC(*p) && size >= blocksize && size < min) {
            dest = p;
            min = size;
            block_size = size;      /* Load this to indicate found block's size */
        }
        
        /* Traverse to the next block */
        p += size;
        size = *(size_t *)p & ~HDR_MASK;
    }
    return dest;
}

/*
 * set_header - set block's header to blocksize marked with flag.
 *      if blocksize is negative, just mark to header's element.
 *      flag should be only HDR_FREE or HDR_ALLOC.
 *      return -1 if header is NULL, 0 if no error.
 */
static int set_header(void *header, size_t blocksize, int flag)
{
    if (header == NULL)
        return -1;
    *(size_t *)header = blocksize | flag;
    return 0;
}

/*
 * set_footer - Duplicate header to foot of block.
 */
static int set_footer(void *header, size_t blocksize)
{
    if (header == NULL)
        return -1;

    size_t *fp;                           /* pointer to footer */

    /* Duplicate footer by header */
    fp = (size_t *)((char *)header + blocksize - sizeof(size_t));
    *fp = *(size_t *)header;
    return 0;
}

/*****************************
 * End of My helper routines *
 *****************************/

/*************************
 *    Debug functions    *
 *************************/

/*************************
 * End of Debug function *
 *************************/
