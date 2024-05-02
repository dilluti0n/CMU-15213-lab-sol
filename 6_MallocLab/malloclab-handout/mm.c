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
 * |<-----------------INIT_SIZE----------------->|
 * |<------INIT_PADD----->|<---sizeof(size_t)--->|
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
 * |<---INIT_SIZE--->|<-----ALIGN(size + sizeof(size_t))----->|
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
 * node_delete_s traverses entire list and delete the node.
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

static void *malloc_existing_block(size_t blocksize);
static void *malloc_new_block(size_t blocksize);
static int   free_coalesce(void *ptr);
static void *get_target_block(size_t blocksize);

inline static void   set_header(void *header, size_t blocksize, int flag);
inline static void   set_footer(void *header, size_t blocksize);
inline static void **node_find_prev(void **curr);
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

/* 32-bit */
#define WORD 4

/* Double word alignment */
#define ALIGNMENT (WORD*2)
#define HDR_MASK  7

/* Round up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~HDR_MASK)

/* Initialize heap with this size and padding */
#define INIT_SIZE ALIGN(WORD)
#define INIT_PADD (INIT_SIZE-WORD)

/* Pointer to first and last block's header */
#define FRST_BLK_HDR ((void *)mem_heap_lo() + INIT_PADD)
#define LAST_BLK_HDR ((void *)mem_heap_hi() - WORD + 1)

/* Changing pointers block-internally. pld: payload */
#define FTR_PTR(ptr_to_next_hdr) ((void *)(ptr_to_next_hdr) - WORD)
#define FTR_ELE(ptr_to_next_hdr) (*(size_t *)FTR_PTR(ptr_to_next_hdr))
#define PLD_PTR(ptr_to_hdr)      ((void *)(ptr_to_hdr) + WORD)
#define HDR_PTR(ptr_to_pld)      ((void *)(ptr_to_pld) - WORD)
#define HDR_ELE(ptr_to_hdr)      (*(size_t *)ptr_to_hdr)

/* Determine header or footer metadatas */
#define IS_ALLOC(hdr_ele) ((hdr_ele) & HDR_ALLOC)
#define IS_PFREE(hdr_ele) ((hdr_ele) & HDR_PFREE)
#define IS_VALID(ftr_ele) ((ftr_ele) & FTR_VALID)

/* Macros for parse size and flag for header element. */
#define MM_SIZE(header) ((header) & ~HDR_MASK)
#define MM_FLAG(header) ((header) & HDR_MASK)

/* flags for function set_header */
#define HDR_FREE  0             /* This block is free */
#define HDR_ALLOC 1             /* This block is allocated */
#define HDR_PFREE 2             /* Previous block is free */
#define FTR_VALID 4             /* Valid footer */

/* #define DEBUG */

#ifdef DEBUG
static void  dbg_heap_check();
static void  dbg_list_check();
static int   dbg_is_on_list(void **ptr);
static void *dbg_next(void **ptr);
static void  dbg_print_heap(int verbose);
static void  dbg_print_list();

#define DBG_CHECK \
    printf("\n------- [%3i]: %s returns -------\n",__LINE__, __func__);\
    dbg_print_heap(0);\
    dbg_print_list();\
    dbg_heap_check();\
    dbg_list_check();\
    printf("-----------------------------------\n");\

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
    if ((p = mem_sbrk(INIT_SIZE)) == (void *)-1)
        return -1;
    p += INIT_PADD; /* Set p as pointer to header */
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
    void         *p;
    const size_t  blksize = ALIGN(size + WORD);

    if ((p = malloc_existing_block(blksize)) != NULL)
        return p;
    return malloc_new_block(blksize);
}

/*
 * mm_free - Freeing a block by passing argument pointer to payload.
 */
void mm_free(void *ptr)
{
    if (free_coalesce(ptr))
        node_top((void **)ptr);

    DBG_CHECK
}

/*
 * mm_realloc - Change the size of the allocation pointed to by ptr to size, and
 *       returns ptr. If there is not enough room to enlarge, it creates a new
 *       allocation, copies the old data, frees the old allocation, and returns
 *       a pointer to the allocated memory.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t        blksize = ALIGN(size + WORD);
    void         *hp      = HDR_PTR(ptr); /* Pointer to header */
    const size_t  hele    = HDR_ELE(hp);
    const size_t  oldsize = MM_SIZE(hele);

    if (blksize == oldsize)
        return ptr;
    if (blksize < oldsize) {
        /* Set blocksize to blk remaining flags */
        HDR_ELE(hp) = hele - blksize + oldsize;

        /* Set next block's header */
        set_header(hp + blksize, oldsize - blksize, HDR_FREE);
        set_footer(hp + blksize, oldsize - blksize);
        return ptr;
    }
    void *nptr = mm_malloc(size);
    memcpy(nptr, ptr, oldsize - WORD);
    mm_free(ptr);
    return nptr;
}

/**********************
 * My helper routines *
 **********************/
/*
 * malloc_existing_block - Try to allocate requested blocksize block from
 *     existing block by calling get_target_block. If there is appropria-
 *     te block, allocate it and return pointer to payload. If not,
 *     return NULL.
 */
static void *malloc_existing_block(size_t blocksize)
{
    void *p;
    /* Traverse the free list. */
    if ((p = get_target_block(blocksize)) != NULL) {
        const size_t surplus = block_size - blocksize; /* Size of remainder space */

        /* Mark target block as use */
        set_header(p, blocksize, HDR_ALLOC);

        /* Mark empty space as free if there is empty space */
        void *nh = p + blocksize;
        if (surplus != 0) {
            set_header(nh, surplus, HDR_FREE);
            set_footer(nh, surplus);
            node_replace(prev_node, (void **)PLD_PTR(p), (void **)PLD_PTR(nh));
        } else {                /* There is no empty space */
            /* Mark next block as "Previous alloc" */
            HDR_ELE(nh) = HDR_ELE(nh) & ~HDR_PFREE;
            node_delete(prev_node, (void **)PLD_PTR(p));
        }
        return PLD_PTR(p);
    }
    return NULL;
}

/*
 * malloc_new_block - Allocate new block by calling mm_sbrk. If there is free
 *        space trailing area of heap, use it that area first and expand heap.
 *        It returns pointer to payload of new block or NULL, when mm_sbrk
 *        fails.
 */
static void *malloc_new_block(size_t blocksize)
{
    /* There is no appropriate free block on the heap. */
    void *p = LAST_BLK_HDR;
    size_t require = blocksize;             /* Requirement of heap */

    /* If previous block is free, new block should start at there. */
    if (IS_PFREE(HDR_ELE(p))) {
        const size_t pbsize = MM_SIZE(FTR_ELE(p)); /* Previous block's size */
        require -= pbsize;
        p -= pbsize;
        node_delete_s((void **)PLD_PTR(p));
    }

    /* Request more heap !! */
    if (mem_sbrk(require) == (void *)-1) {
        return NULL;
    }
    set_header(p, blocksize, HDR_ALLOC);    /* Set header as (blocksize)/01 */
    set_header(p + blocksize, 0, HDR_ALLOC); /* Set header of last block as 0/01 */

    return PLD_PTR(p);    /* return pointer to payload */
}

/*
 * free_coalesce - When freeing the block, coalesce with the previous or next
 *      adjacent block if it is free. This function sets the header and footer
 *      of the block, and returns 1 if the coalesce is happend, 0 if not.
 */
int free_coalesce(void *ptr)
{
    void         **curr    = (void **)ptr; /* Pointer to node */
    void          *p       = HDR_PTR(ptr); /* Pointer to header */
    const size_t   hele    = HDR_ELE(p); /* Header element */
    size_t         blksize = MM_SIZE(hele); /* Blocksize */

    int isnotmerged = 1;

    /* Merge with previous block */
    if (IS_PFREE(hele)) {
        size_t ftr = FTR_ELE(p); /* Header element of previous block */

        /* Only 8-byte block's footer is overrided by list node. */
        if (!IS_VALID(ftr))
            ftr = *((size_t *)p - 2);

        /* Merge two blocks */
        blksize += MM_SIZE(ftr);
        p -= MM_SIZE(ftr);

        isnotmerged = 0;
    }

    /* Merge with next block */
    void         *nh    = p + blksize; /* Pointer to next block's header */
    const size_t  nhele = HDR_ELE(nh); /* Header element of next block */
    if (!IS_ALLOC(nhele)) {
        void **next = (void **)PLD_PTR(nh); /* Pointer to next block's node */
        if (isnotmerged) { /* prev alloc, next free */
            if (MM_SIZE(nhele) > 8) {
                /* Defer process to get_target_block. See header comment. */
                node_insert(next, curr);
                HDR_ELE(nh) = -1U;
            } else {
                /**
                 * TODO: improve this to not search entire list.
                 *
                 * If size of the next block is less then 8, footer of curr
                 * overwrites node "next".
                 */
                node_replace_s(next, curr);
            }
        } else { /* prev free, next free */
            node_delete_s(next);
        }
        blksize += MM_SIZE(nhele);

        isnotmerged = 0;
    }

    /* Set current block's header and footer */
    set_header(p, blksize, HDR_FREE);
    set_footer(p, blksize);

    /* Set non-merged next block's header */
    void *const np = p + blksize;           /* Pointer to next block's header */
    set_header(np, MM_SIZE(HDR_ELE(np)), HDR_PFREE | HDR_ALLOC);

    return isnotmerged;
}

/*
 * get_target_block - return the pointer to head of appropriate free block while
 *       traversing the explicit free list. return NULL if there is no space.
 *
 *       Set global variable block_size as the founded block's size to specify
 *       the size of empty block after allocate.
 */
static void *get_target_block(size_t blocksize)
{
    size_t   size;            /* Blocksize of each iteration. */
    size_t   min  = -1U;      /* Minimum blocksize for entire iteration */
    void    *dest = NULL;     /* Pointer to header of target block */
    void   **curr = TOP;      /* Pointer to block's node of current iteration */
    void   **prev = (void **)&TOP; /* Previous node of current ineration */

    while (curr != NULL) {
        void   *hp   = HDR_PTR(curr); /* Pointer to header */
        size_t  hele = HDR_ELE(hp); /* Header element of each block */

        /**
         * Resolve prev -> p -> *p -> (n) to prev -> *p -> (n) and move
         * current iteration to *p. (which is real adress to block node)
         */
        while (hele == -1U) {
            node_delete(prev, curr);
            if ((curr = *curr) == NULL)
                return dest;
            hp   = HDR_PTR(curr);
            hele = HDR_ELE(hp);
        }
        size = MM_SIZE(hele);

        /* If the block is appropriate, load hp to dest */
        if (size >= blocksize && size < min) {
            dest = hp;
            min = size;
            prev_node = prev;
            block_size = size;    /* Load this to indicate found block's size */
        }

        /* Traverse to the next list node */
        prev = curr;
        curr = *curr;
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
inline static void set_header(void *header, size_t blocksize, int flag)
{
    *(size_t *)header = blocksize | flag;
}

/*
 * set_footer - Duplicate header to last `size_t` part of block.
 */
inline static void set_footer(void *header, size_t blocksize)
{
    size_t *fp;                           /* pointer to footer */

    /* Duplicate header to footer */
    fp = (size_t *)(header + blocksize - WORD);
    *fp = (*(size_t *)header) | FTR_VALID;
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

/*
 * node_find_prev - Find previouse node of curr by traverse entire list. This
 *          function assume that node curr is in the list. If not, it whould
 *          not stop...
 *          If macro DEBUG is defined, in the caase that curr is not included
 *          inside of list, the program would be exited with code 1.
 */
inline static void **node_find_prev(void **curr)
{
    void **p = TOP, **prev = (void **)&TOP;
    while (p != curr) {
#ifdef DEBUG
        if (p == NULL) {
            fprintf(stderr,
               "fatal error: attempt to search node %p but it is not on list\n"\
               "header: %x\n", curr, *(size_t *)(curr - 1));
            exit(1);
        }
#endif
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
    void **prev = node_find_prev(curr);
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
    void **prev = node_find_prev(src);
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
    void *p = FRST_BLK_HDR;
    size_t size = MM_SIZE(HDR_ELE(p));
    void **node = TOP;

    while (size != 0) {
        if (!IS_ALLOC(*(size_t *)p)) {
            if (!dbg_is_on_list((void **)p + 1)) {
                fprintf(stdout, "[%p]: (%p) %u; is not on list.\n",
                        p, PLD_PTR(p), MM_SIZE(HDR_ELE(p));
                dbg_print_list();
                dbg_print_heap(0);
                exit(3);
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
    void *p = FRST_BLK_HDR;
    size_t size = MM_SIZE(HDR_ELE(p));

    printf("---HEAP---\n");
    while (size != 0) {
        size_t hele = HDR_ELE(p);
        if (verbose || !IS_ALLOC(hele))
            printf("[%p]: (%p) %4u; (%u,%u)\n",
                   p, PLD_PTR(p), size, IS_PFREE(hele),
                   IS_ALLOC(hele));
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
