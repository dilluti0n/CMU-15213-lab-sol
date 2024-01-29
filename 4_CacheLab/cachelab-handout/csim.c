/*
 * Dilluti0n
 * hskimse1@gmail.com
 */
#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#define MAXLINE 255

enum res {
    err = -1,
    miss = 0,
    hit,
    eviction,
};

struct {
    int verbose;
    int s;
    int E;
    int b;
    FILE *fp;
} _op; /* inline argument options */

struct {
    int hit;
    int miss;
    int eviction;
} _count; /* result */

typedef struct {
    char id;
    size_t tag;
    size_t set_i;
} c_ele_;

struct cache_line {
    int v;
    size_t tag;
    struct cache_line *next;
};

struct cache_set {
    int cnt;
    struct cache_line *head;
    struct cache_line *tail;
};

int get_line(char *line, int lim, FILE *fp);
int execute_line(char *line, struct cache_set *cache);
void parse_adress_bit(size_t adress, c_ele_ *ad);
void search_cache(struct cache_set *set, size_t tag);
void load_result(enum res result);
void inc_res(enum res result);
struct cache_line *add_tail(struct cache_set *q);
void rmv_head(struct cache_set *q);
int move_node_to_tail(struct cache_set *q, struct cache_line *prev);
void free_cache(struct cache_set cache[], int size);
void free_quene(struct cache_set *q);
void parse_args(int argc, char *argv[]);
void usage(char *name);

int main(int argc, char *argv[])
{
    _op.fp = stdin;
    parse_args(argc, argv);
    const int c_size = 1 << _op.s; /* 2^s */
    char line[MAXLINE];
    struct cache_set cache[c_size]; /* cache[2^s]*/
    memset(cache, 0, sizeof(cache)); /* initalize cache */
    while (get_line(line, MAXLINE, _op.fp) > 0)
        execute_line(line, cache);
    if (_op.fp != stdin)
        fclose(_op.fp);
    printSummary(_count.hit, _count.miss, _count.eviction);
    free_cache(cache, c_size);
    exit(0);
}

int get_line(char *line, int lim, FILE *fp)
{
    int c;
    char *init = line;
    while ((c = fgetc(fp)) != '\n' && c != EOF && lim-- > 0)
        *line++ = c;
    *line = '\0';
    if (c == EOF)
        return c;
    return line - init;
}


int execute_line(char *line, struct cache_set cache[]) {
    size_t adress;
    c_ele_ ad;
    if (*line++ != ' ')
        return -1;
    sscanf(line, "%c %lx,", &ad.id, &adress);
    parse_adress_bit(adress, &ad);
    if (_op.verbose != 0) {
        printf("%s", line);
        printf(" set%lu tag(%#lx) :", ad.set_i, ad.tag);
    }
    search_cache(&cache[ad.set_i], ad.tag);
    if (ad.id == 'M')
        search_cache(&cache[ad.set_i], ad.tag);
    if (_op.verbose != 0)
        printf("\n");
    return 0;
}

void parse_adress_bit(size_t adress, c_ele_ *ad)
{
    ad->set_i = adress >> _op.b & ~(-1 << _op.s); /* lower s+b to b bits */
    ad->tag = adress >> (_op.s + _op.b); /* except b and s bits */
}

void search_cache(struct cache_set *set, size_t tag)
{
    struct cache_line *p, *prev = NULL;
    if (set == NULL)
        return;
    for (p = set->head; p != NULL && p->tag != tag; prev = p, p = p->next)
        ;
    if (p == NULL) {
        if ((p = add_tail(set)) == NULL) {
            fprintf(stderr, "err: allocation failed\n");
            exit(-1);
        }
        p->v = 1;
        p->tag = tag;
        inc_res(miss); /* miss */
        if (_op.E < set->cnt) {
            rmv_head(set);
            inc_res(eviction); /* eviction */
        }
    } else if (p->v == 0) {
        p->v = 1;
        p->tag = tag;
        inc_res(miss); /* miss */
    } else {
        move_node_to_tail(set, prev);
        inc_res(hit); /* hit */
    }
}

/* for verbose mode */
void inc_res(enum res result)
{
    switch (result) {
    case miss:
        _count.miss++;
        if (_op.verbose != 0)
            printf(" miss");
        break;
    case hit:
        _count.hit++;
        if (_op.verbose != 0)
            printf(" hit");
        break;
    case eviction:
        _count.eviction++;
        if (_op.verbose != 0)
            printf(" eviction");
        break;
    case err:
        break;
    }
}

struct cache_line *add_tail(struct cache_set *q)
{
    struct cache_line *newt;
    if (q == NULL)
        return NULL;
    if ((newt = malloc(sizeof(struct cache_line))) == NULL)
        return NULL;
    if (q->head == NULL) {
        q->tail = q->head = newt;
    } else {
        q->tail->next = newt;
        q->tail = newt;
    }
    q->cnt++;
    newt->v = 0;
    newt->tag = 0;
    newt->next = NULL;
    return newt;
}

void rmv_head(struct cache_set *q)
{
    if (q != NULL && q->head != NULL) {
        struct cache_line *newh = q->head->next;
        free(q->head);
        q->head = newh;
        q->cnt--;
    }
}

int move_node_to_tail(struct cache_set *q, struct cache_line *prev)
{
    struct cache_line *newt;
    if (q == NULL || q->head == NULL) /* if quene is NULL or empty */
        return -1;
    if (prev != NULL) { /* newt is not head. */
        newt = prev->next;
        if (newt == NULL)
            return -1; /* this is not intended. */
        if (newt->next == NULL) /* newt is tail */
            return 0;
        prev->next = newt->next;
    } else { /* newt is head */
        newt = q->head;
        if (newt->next != NULL) /* else, head should not be changed. */
            q->head = newt->next;
    }
    q->tail->next = newt;
    q->tail = newt;
    newt->next = NULL; /* set newt as tail. */
    return 0;
}

void free_cache(struct cache_set cache[], int size)
{
    int i;
    for (i = 0; i < size; i++)
        free_quene(&cache[i]);
}

void free_quene(struct cache_set *q)
{
    if (q != NULL && q->head != NULL) {
        struct cache_line *p, *next;
        for (p = q->head; p != NULL; p = next) {
            next = p->next;
            free(p);
        }
    }
}

void parse_args(int argc, char *argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1)
        switch(ch) {
        case 'h':
            usage(argv[0]);
            break;
        case 'v':
            _op.verbose = 1;
            break;
        case 's':
            _op.s = atoi(optarg);
            break;
        case 'E':
            _op.E = atoi(optarg);
            break;
        case 'b':
            _op.b = atoi(optarg);
            break;
        case 't':
            if ((_op.fp = fopen(optarg, "r")) == NULL) {
                fprintf(stderr, "err: invalid path %s\n", optarg);
                exit(2);
            }
            break;
        case '?':
        default:
            fprintf(stderr, "err: invalid argument %c\n", optopt);
            usage(argv[0]);
            break;
        }
    if (_op.s == 0 || _op.E == 0 || _op.b == 0)
        usage(argv[0]);
}

void usage(char *name)
{
    printf("usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    exit(1);
}
