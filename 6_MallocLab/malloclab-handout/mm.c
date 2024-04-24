/**
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
 * ----FLAGS----
 * header element - `size_t` value that loaded on the header of block.
 *      Each header elements are represented by (size)/(pfree)(alloc).
 * (size)  - Size of block.
 * (pfree) - Flag that indicates either previous block is free(1) or
 *    allocated(0). Macro `HDR_PFREE` is flag for pfree.
 * (alloc) - Flag that indicates either current block is allocated(1) or
 *    free(0). Macro `HDR_ALLOC` is flag for alloc.
 * The precise useage of each macros `HDR_FREE`, `HDR_PFREE`, `HDR_ALLOC`
 * is defined on the comment of function `set_header`.
 * ----FOOTER----
 * Note that on this structure, every free block has `footer`, exact duplicate
 * of its header, and located on the last `size_t` section of the block.
 * It can be easily duplicated by function `set_footer`.
 *
 * The footer is used when (pfree) flag is turned on to next block, which is
 * crucial for allocater, to indicate previous block's size.
 */

/*
   TODO: Deal with binary-bal.rep (which allocates 448 64, free 448 and then
         allocates 512.) << HOW??
         Implement heap checker
         Implement Explicit free list (free() has been done, malloc(), i.e.
         get_target_block() should be modified using explicit list.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static int block_size;          /* Size of block founded by get_target_block */
static void **TOP;              /* Top node of explicit free list */
static void **BOT;              /* Bottom node for explicit free list */

static void *get_target_block(size_t blocksize);
inline static int set_header(void *header, size_t blocksize, int flag);
inline static int set_footer(void *header, size_t blocksize);

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

/* Macros for parse size and flag for header element. */
#define MM_SIZE(header) ((header)&~HDR_MASK)
#define MM_FLAG(header) ((header)&HDR_MASK)

/* flags for function set_header */
#define HDR_FREE 0
#define HDR_ALLOC 1         /* This block is allocated */
#define HDR_PFREE 2           /* Previous block is free */

#define DEBUG

#ifdef DEBUG

static void dbg_heap_check();
static void dbg_list_check();
static int dbg_is_on_list(void **ptr);
static void *dbg_next(void **ptr);
static void dbg_print_heap(int verbose);
static void dbg_print_list();

#define DBG_CHECK \
    printf("[%3i]: %s returns\n",__LINE__, __func__);\
    dbg_print_heap(1);                           \
    dbg_print_list();\
    dbg_heap_check();\

#else
#define DBG_CHECK
#endif


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Initialize global variables */
    block_size = 0;
    BOT = (void **)&TOP;
    TOP = NULL;

    /* allocate initial block */
    void *p;
    if ((p = mem_sbrk(SIZE_T_SIZE)) == (void *)-1)
        return -1;
    p += SIZE_T_PADDING; /* Set p as pointer to header */
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
                                               /* Block size to malloc */
    const size_t blksize = ALIGN(size + sizeof(size_t));

    /* Traverse the free list. */
    if ((p = get_target_block(blksize)) != NULL) {
        const size_t tmp = block_size - blksize;       /* Size of empty space */

        /* Mark target block for use */
        set_header(p, blksize, HDR_ALLOC);

        /* Mark empty space as free if there is empty space */
        void *next = p + blksize;
        if (tmp != 0) {
            set_header(next, (size_t) tmp, HDR_FREE);
            set_footer(next, (size_t) tmp);
        } else {                /* There is no empty space */
            /* Mark next block as "Previous alloc" */
            *(size_t *)next = *(size_t *)next & ~HDR_PFREE;
        }
        DBG_CHECK
        return (void *)((size_t *)p + 1);        /* Return pointer to payload */
    }

    /* There is no appropriate free block on the heap. */
    p = (void *)mem_heap_hi() - sizeof(size_t) + 1;
    size_t excess = blksize;             /* Requirement of heap */

    /* If previous block is free, new block should start at there. */
    if (IS_PFREE(*(size_t *)p)) {
                                                     /* Previous block's size */
        const size_t psize = MM_SIZE(*((size_t *)p - 1));
        excess -= psize;
        p -= psize;                       /* New block should start at here. */
    }

    /* Request more heap !! */
    if (mem_sbrk(excess) == (void *)-1) {
        return NULL;
    }
    set_header(p, blksize, HDR_ALLOC);    /* Set header as (blksize)/01 */
    set_header(p + blksize, 0, HDR_ALLOC); /* Set header of last block as 0/01 */

    DBG_CHECK
    return (void *)((size_t *)p + 1);    /* return pointer to payload */
}

/*
 * mm_free - Freeing a block by passing argument pointer to payload.
 */
void mm_free(void *ptr)
{
    void **curr = (void **)ptr;              /* Pointer to explicit list node */
    void *p = (size_t *)ptr - 1;             /* Pointer to header */
    const size_t hdr = *(size_t *)p;         /* Header element */
    size_t blksize = MM_SIZE(hdr);           /* Blocksize */

    int isnotmerged = 1;

    /* Merge with previous block */
    size_t tmp;
    if (IS_PFREE(hdr)) {
        tmp = *((size_t *)p - 1);        /* Header element of previous block */

        /* Set flag and blocksize and header */
        const size_t pbsize = MM_SIZE(tmp); /* Size of previous block */
        blksize += pbsize;
        p -= pbsize;            /* Merge two blocks */

        /* We can consider prev block's node as our node */
        curr = (void **)((void *)curr - pbsize);

        isnotmerged = 0;
    }

    /* Merge with next block */
    tmp = *(size_t *)(p + blksize);   /* Header element of next block */
    if (!IS_ALLOC(tmp)) {
        blksize += MM_SIZE(tmp);
                                         /* Pointer to next block's list node */
        void **next = (void *)p + blksize + sizeof(size_t);
        *curr = *next;
        /* If current block has not been merged with prev block, construct
         * the linked list like (p) -> next -> curr -> (n) from the origi-
         * -nal one (p) -> next -> (n), mark the unused next block's header
         * as -1, and defer the handling to the function get_target_block
         * since we cannot access to (p) here.
         */
        if (isnotmerged) {
            *next = (void *)curr;
            *(size_t *)(p + blksize) = -1U;
        }

        isnotmerged = 0;
    }

    /* Set this block's marking */
    set_header(p, blksize, HDR_FREE);
    set_footer(p, blksize);

    /* Set next block's marking */
    void *np = p + blksize;                /* Pointer to next block's header */
    set_header(np, MM_SIZE(*(size_t *)np), HDR_PFREE | HDR_ALLOC);

    /* If curr block is not merged, append it to BOT of explicit free list */
    if (isnotmerged) {
        *curr = NULL;
        *BOT = curr;            /* Note: BOT is initialized as &TOP */
        BOT = curr;             /* Mark curr as bottom */
    }
    DBG_CHECK
}

/*
 * mm_realloc - Change the size of the allocation pointed to by ptr to size, and
 *       returns ptr. If there is not enough room to enlarge, it creates a new
 *       allocation, copies the old data, frees the old allocation, and returns
 *       a pointer to the allocated memory.
 */
/* TODO: this fails with explit free list implimentation for free() */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    size_t blksize = ALIGN(size + sizeof(size_t));
    void *hp = (size_t *)ptr - 1; /* Pointer to header */
    const size_t header = *(size_t *)hp;
    const size_t oldsize = MM_SIZE(header);

    if (blksize == oldsize)
        return ptr;
    if (blksize < oldsize) {
        /* Set blocksize to blk remaining flags */
        *(size_t *)hp = header - blksize + oldsize;

        /* Set next block's header */
        set_header(hp + blksize, oldsize - blksize, HDR_FREE);
        set_footer(hp + blksize, oldsize - blksize);
        return ptr;
    }
    void *nptr = mm_malloc(size);
    memcpy(nptr, ptr, oldsize - sizeof(size_t));
    mm_free(ptr);
    return nptr;
}

/**********************
 * My helper routines *
 **********************/

/*
 * get_target_block - return the pointer to head of appropriate free block while
 *       traversing the implicit list. return NULL if there is no space.
 *
 *       Set global variable block_size as the founded block's size to specify
 *       the size of empty block after allocate.
 */
static void *get_target_block(size_t blocksize)
{
                                            /* Pointer to each block's header */
    char *p = (char *)mem_heap_lo() + SIZE_T_PADDING;
    size_t size = MM_SIZE(*(size_t *)p);   /* Blocksize of current iteration. */
    void *dest = NULL;                        /* Target block. */
    size_t min = -1U;              /* Minimum blocksize for entire iteration */

    while (size != 0) {
        /* If p is the appropriate block, load it to dest */
        if (!IS_ALLOC(*p) && size >= blocksize && size < min) {
            dest = p;
            min = size;
            block_size = size;    /* Load this to indicate found block's size */
        }

        /* Traverse to the next block */
        p += size;
        size = MM_SIZE(*(size_t *)p);
    }
    return dest;
}

/*
 * set_header - set block's header to blocksize marked with flag.
 *      Return -1 if header is NULL, 0 if no error.
 *
 *        flag         previous block / current block
 * ---------------------------------------------------
 *      HDR_FREE          allocated       free
 *      HDR_PFREE           free          free
 *      HDR_ALLOC         allocated     allocated
 * HDR_PFREE|HDR_ALLOC      free        allocated
 */
inline static int set_header(void *header, size_t blocksize, int flag)
{
    if (header == NULL)
        return -1;
    *(size_t *)header = blocksize | flag;
    return 0;
}

/*
 * set_footer - Duplicate header to last `size_t` part of block.
 */
inline static int set_footer(void *header, size_t blocksize)
{
    if (header == NULL)
        return -1;

    size_t *fp;                           /* pointer to footer */

    /* Duplicate header to footer */
    fp = (size_t *)(header + blocksize - sizeof(size_t));
    *fp = *(size_t *)header;
    return 0;
}

/*****************************
 * End of My helper routines *
 *****************************/

/*************************
 *    Debug functions    *
 *************************/
#ifdef DEBUG

static void dbg_heap_check()
{
    void *p = mem_heap_lo() + SIZE_T_PADDING;
    size_t size = MM_SIZE(*(size_t *)p);
    void **node = TOP;

    while (size != 0) {
        if (!IS_ALLOC(*(size_t *)p)) {
            if (!dbg_is_on_list((void **)p + 1)) {
                fprintf(stdout, "[%p]: (%p) %lu; is not on list.\n",
                        p, p + sizeof(size_t), MM_SIZE(*(size_t *)p));
                dbg_print_list();
                dbg_print_heap(0);
                exit(-1);
            }
            node = (void **)dbg_next(node);
        }
        p += size;
        size = MM_SIZE(*(size_t *)p);
    }
}

static int dbg_is_on_list(void **ptr)
{
    void **node = TOP;
    while (node != NULL) {
        if (ptr == node)
            return 1;
        node = (void **)dbg_next(node);
    }
    return 0;
}

static void *dbg_next(void **ptr)
{
    if (ptr == NULL)
        return (void *)-1;

    void *next = *ptr;
    if (next == NULL)
        return NULL;
    if (*((size_t *)next - 1) == -1)
        return *(void **)next;
    return next;
}

static void dbg_print_heap(int verbose) {
    void *p = mem_heap_lo() + SIZE_T_PADDING;
    size_t size = MM_SIZE(*(size_t *)p);

    printf("---HEAP---\n");
    while (size != 0) {
        size_t header = *(size_t *)p;
        if (verbose || !IS_ALLOC(header))
            printf("[%p]: (%p) %4lu; (%lu,%lu)\n",
                   p, p + sizeof(size_t), size, IS_PFREE(header),
                   IS_ALLOC(header));
        p += size;
        size = MM_SIZE(*(size_t *)p);
    }

}

static void dbg_print_list()
{
    printf("---Free list---\n");
    void **node = TOP;
    printf("TOP = ");
    while (node != NULL) {
        printf("(%p) -> ", node);
        if (node == dbg_next(node)) {
            printf("<- \n");
            exit(1);
        }
        node = dbg_next(node);
    }
    if (node == NULL)
        printf("NULL\n");
}

#endif
/*************************
 * End of Debug function *
 *************************/
