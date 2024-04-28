/**
 * mm.c - memory allocation module using explicit free list.
 *
 * Usable but needs improvement for certain cases.
 * ----
 * NOTE: This code is for 32-bit system; Compile it with -m32 flag.
 * ----ALIGNMENT----
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
 *
 * ----FLAGS FOR HEADER----
 * header element - `size_t` value that loaded on the header of block.
 *      Each header elements are represented by (size)/(pfree)(alloc).
 * (size)  - Size of block.
 * (pfree) - Flag that indicates either previous block is free(1) or
 *    allocated(0). Macro `HDR_PFREE` is flag for pfree.
 * (alloc) - Flag that indicates either current block is allocated(1) or
 *    free(0). Macro `HDR_ALLOC` is flag for alloc.
 * The precise useage of each macros `HDR_FREE`, `HDR_PFREE`, `HDR_ALLOC`
 * is defined on the comment of function `set_header`.
 *
 * ----EXPLICIT FREE LIST----
 * The global variable TOP contains pointer to head of list node. Each list
 * nodes for each blocks are alligned on next eight bytes of header. So th-
 * -at, pointer to next node is given by *(void **)curr, when curr is
 * pointer to current node. By this rule, if blocksize is 8, storing
 * that block to list would override the footer. This can be resolv-
 * -ed by using flag FTR_VALID, which is described on next section.
 *
 * mm_free merge with prev and next block on heap, if either one is free.
 * However this is not as simple as implicit list because we do not know
 * (a)where the prev or next block is located on the free list, and
 * (b)value of pointer to previous node. So that we should handle
 * the four cases diffrently in here.
 *
 * 1) prev alloc, next free
 * Modify list (p) -> next -> (n) to (p) -> next -> curr -> (n), and mark
 * header of prev as -1. This tided state would be resolved when traver-
 * -sing list on get_target_block. If size of next is less then 8,
 * pointer to curr is stored on the last 8 bytes of block that node curr
 * is placed, so setting footer for this block overrides it. Thus if
 * blocksize is 8, we will traverse list and replace next to
 * curr instead.
 *
 * 2) prev free, next free
 * Delete the next node on the list. This will consume extra resource since
 * node_replace_s traverses entire list and replace the node.
 *
 * 3) prev free, next alloc
 * In this case, there is nothing to do.
 *
 * 4) prev alloc, next alloc
 * Append current block to the TOP of list by calling node_top.
 *
 * ----FOOTER----
 * Note that on this structure, every free block has `footer`, exact duplicate
 * of its header, and located on the last `size_t` section of the block.
 * It can be easily duplicated by function `set_footer`.
 *
 * The footer is used when (pfree) flag is turned on to next block, which is
 * crucial for allocater, to indicate previous block's size.
 *
 * Flag FTR_VALID is used for footer. Its defined as '4'. Since we using double
 * word alignment, every three low bits of each elements for list nodes are
 * zero. So when footer is overrided by list node, this flag will be over-
 * rided. And we place list nodes as next to the header, so this case is
 * occured only if the block's size is 8 byte.
 */

/*
   TODO: Deal with binary-bal.rep (which allocates 448 64, free 448 and then
         allocates 512.) << HOW??
         Implement segregated free list.
         Increase readability and modurarility by defining some macros.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static int    block_size;       /* Size of block founded by get_target_block */
static void **prev_node;        /* Pointer to previous free block's node */
static void **TOP;              /* Top node of explicit free list */

static void         *get_target_block(size_t blocksize);
inline static int    set_header(void *header, size_t blocksize, int flag);
inline static int    set_footer(void *header, size_t blocksize);
inline static void **node_search_prev(void **curr);
inline static void   node_delete(void **prev, void **curr);
inline static void   node_delete_s(void **curr);
inline static void   node_replace(void **prev, void **src, void **dst);
inline static void   node_replace_s(void **src, void **dst);
inline static void   node_top(void **node);
inline static void   node_insert(void **prev, void **node);

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
#define IS_VALID_FTR(footer) ((footer)&FTR_VALID)

/* Macros for parse size and flag for header element. */
#define MM_SIZE(header) ((header)&~HDR_MASK)
#define MM_FLAG(header) ((header)&HDR_MASK)

/* flags for function set_header */
#define HDR_FREE 0
#define HDR_ALLOC 1         /* This block is allocated */
#define HDR_PFREE 2         /* Previous block is free */

#define FTR_VALID 4

//#define DEBUG

#ifdef DEBUG

static void dbg_heap_check();
static void dbg_list_check();
static int dbg_is_on_list(void **ptr);
static void *dbg_next(void **ptr);
static void dbg_print_heap(int verbose);
static void dbg_print_list();

#define DBG_CHECK \
    printf("[%3i]: %s returns\n",__LINE__, __func__);\
    dbg_print_heap(0);\
    dbg_print_list();\
    dbg_heap_check();\
    dbg_list_check();

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

        /* Mark target block as use */
        set_header(p, blksize, HDR_ALLOC);

        /* Mark empty space as free if there is empty space */
        void *next = p + blksize;
        if (tmp != 0) {
            set_header(next, (size_t)tmp, HDR_FREE);
            set_footer(next, (size_t)tmp);
            node_replace(prev_node, (void **)p + 1, (void **)next + 1);
        } else {                /* There is no empty space */
            /* Mark next block as "Previous alloc" */
            *(size_t *)next = *(size_t *)next & ~HDR_PFREE;
            node_delete(prev_node, (void **)p + 1);
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
        node_delete_s((void **)(p + sizeof(size_t)));
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
    if (IS_PFREE(hdr)) {
        size_t tmp;
        tmp = *((size_t *)p - 1); /* Header element of previous block */
        if (!IS_VALID_FTR(tmp))
            /* issue; Only 8-byte block's footer is overrided by list node. */
            tmp = *((size_t *)p - 2);

        /* Set flag and blocksize and header */
        const size_t pbsize = MM_SIZE(tmp); /* Size of previous block */
        blksize += pbsize;
        p -= pbsize;            /* Merge two blocks */
        isnotmerged = 0;
    }

    /* Merge with next block */
    void *nh = p + blksize;                  /* Pointer to next block's header */
    size_t nhele = *(size_t *)nh;            /* Header element of next block */
    if (!IS_ALLOC(nhele)) {
        /* Pointer to next block's list node */
        void **next = nh + sizeof(size_t);
        if (isnotmerged) {
            /**
             * prev alloc, next free
             * Construct the list like (p) -> next -> curr -> (n) from the origi-
             * -nal one (p) -> next -> (n), mark the unused next block's header
             * as -1, and defer the handling to the function get_target_block
             * since we cannot access to (p) here.
             */
            if (MM_SIZE(nhele) > 8) {
                node_insert(next, curr);
                *(size_t *)nh = -1U;
            } else {
                /* TODO: improve this to not search entire list */
                /* This case, footer of curr block will overlap node "next" */
                node_replace_s(next, curr);
            }
        } else {
            /**
             * prev free, next free
             * Just deleting next block on free list would be fine.
             */
            node_delete_s(next);
        }
        blksize += MM_SIZE(nhele);

        isnotmerged = 0;
    }

    /* Set this block's marking */
    set_header(p, blksize, HDR_FREE);
    set_footer(p, blksize);

    /* Set next block's marking */
    void *np = p + blksize;                /* Pointer to next block's header */
    set_header(np, MM_SIZE(*(size_t *)np), HDR_PFREE | HDR_ALLOC);

    /* If curr block is not merged, append it to TOP of free list */
    if (isnotmerged) /* prev alloc, next alloc */
        node_top(curr);

    /* prev free, next alloc :Do nothing. */

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
 *       traversing the explicit free list. return NULL if there is no space.
 *
 *       Set global variable block_size as the founded block's size to specify
 *       the size of empty block after allocate.
 */
static void *get_target_block(size_t blocksize)
{
    void **p = TOP;                 /* Pointer to each free block's node */
    size_t size;                    /* Blocksize of each iteration. */
    void *dest = NULL;              /* Pointer to header of target block */
    size_t min = -1U;               /* Minimum blocksize for entire iteration */
    void **prev = (void **)&TOP;    /* Previous node for current ineration */

    while (p != NULL) {
        void *hp = (void *)((size_t *)p - 1);     /* Pointer to header */
        size_t header = *(size_t *)hp;  /* Header element of each block */

        /**
         * Resolve prev -> p -> *p -> (n) to prev -> *p -> (n) and move
         * current iteration to *p. (which is real adress to block node)
         */
        while (header == -1U) {
            node_delete(prev, p);
            if ((p = *p) == NULL)
                return dest;
            hp = (void *)(p - 1);
            header = *(size_t *)hp;
        }
        size = MM_SIZE(header);

        /* If the block is appropriate, load hp to dest */
        if (size >= blocksize && size < min) {
            prev_node = prev;
            dest = hp;
            min = size;
            block_size = size;    /* Load this to indicate found block's size */
        }

        /* Traverse to the next list node */
        prev = p;
        p = *p;
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
    *fp = (*(size_t *)header) | FTR_VALID;
    return 0;
}

/*
 * node_top - Put node to the TOP of list.
 */
inline static void node_top(void **node)
{
    *node = TOP;
    TOP = node;
}

/*
 * node_insert - insert node next to the prev node. prev should not be NULL.
 */
inline static void node_insert(void **prev, void **node)
{
    *node = *prev;
    *prev = node;
}

inline static void **node_search_prev(void **curr)
{
    void **p = TOP, **prev = (void **)&TOP;
    while (p != curr) {
        if (p == NULL) {
            fprintf(stderr,
               "fatal error: attempt to search node %p but it is not on list\n"\
               "header: %x\n", curr, *(size_t *)(curr - 1));
            exit(1);
        }
        prev = p;
        p = *p;
    }
    return prev;
}
/*
 * node_delete - Delete current node from explicit free list. prev should
 *  be always non-null, initialized as &TOP. (i.e. if TOP = curr, call of
 *  this function makes TOP = *curr.)
 */
inline static void node_delete(void **prev, void **curr)
{
    *prev = *curr;
}

inline static void node_delete_s(void **curr)
{
    void **prev = node_search_prev(curr);
    node_delete(prev, curr);
}

/*
 * node_replace - replace list node src by dst.
 */
inline static void node_replace(void **prev, void **src, void **dst)
{
    *prev = dst;
    *dst = *src;
}

inline static void node_replace_s(void **src, void **dst)
{
    void **prev = node_search_prev(src);
    node_replace(prev, src, dst);
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
                fprintf(stdout, "[%p]: (%p) %u; is not on list.\n",
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

static void dbg_list_check()
{
    return;
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
            printf("[%p]: (%p) %4u; (%u,%u)\n",
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
