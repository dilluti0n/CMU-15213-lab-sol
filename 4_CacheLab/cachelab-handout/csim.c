#include "cachelab.h"
#include <stdio.h>
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
    int size;
    unsigned tag;
    unsigned set_i;
    unsigned block_i;
} c_ele_;

struct cache_line {
    int v;
    int size;
    unsigned tag;
    unsigned block_i;
    struct cache_line *next;
};

struct cache_set {
    int cnt;
    struct cache_line *head;
    struct cache_line *tail;
} *S;

int get_line(char *line, int lim, FILE *fp);
int execute_line(char *line);
void parse_adress_bit(unsigned adress, c_ele_ *ad);
enum res search_cache(struct cache_set *set, c_ele_ *ad);
void load_result(enum res result);
void inc_res(enum res result);
struct cache_line *add_tail(struct cache_set *q);
void rmv_head(struct cache_set *q);
void parse_args(int argc, char *argv[]);
void useage(char *name);

int main(int argc, char *argv[])
{
    char line[MAXLINE];
    _op.fp = stdin;
    parse_args(argc, argv);
    if (!(S = calloc(1 << _op.s, sizeof(struct cache_set)))) /* S[2^s] */
        exit(3);
    while (get_line(line, MAXLINE, _op.fp) > 0)
        execute_line(line);
    if (_op.fp != stdin)
        fclose(_op.fp);
    printSummary(_count.hit, _count.miss, _count.eviction);
    free(S);
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


int execute_line(char *line) {
    unsigned adress;
    c_ele_ ad;
    enum res result;
    if (*line++ != ' ')
        return -1;
    sscanf(line, "%c %x, %d", &ad.id, &adress, &ad.size);
    parse_adress_bit(adress, &ad);
    if (_op.verbose != 0)
        printf("%s", line);
    result = search_cache(S + ad.set_i, &ad);
    load_result(result);
    if (result != hit && ad.id != 'L')
        load_result(search_cache(S + ad.set_i, &ad));
    if (_op.verbose != 0)
        printf("\n");
    return 0;
}

void parse_adress_bit(unsigned adress, c_ele_ *ad)
{
    ad->block_i = adress & ~(-1 << _op.b); /* lower b bits */
    ad->set_i = adress >> _op.b & ~(-1 << _op.s); /* lower s+b to b bits */
    ad->tag = adress >> (_op.s + _op.b); /* except b and s bits */
}

enum res search_cache(struct cache_set *set, c_ele_ *ad)
{
    struct cache_line *p;
    if (set == NULL)
        return err;
    for (p = set->head; p != NULL && p->tag != ad->tag; p = p->next)
        ;
    if (p == NULL) {
        if ((p = add_tail(set)) < 0) {
            fprintf(stderr, "err: allocation failed\n");
            exit(-1);
        }
        p->v = 1;
        p->size = ad->size;
        p->tag = ad->tag;
        p->block_i = ad->block_i;
        if (set->cnt < _op.E) {
            set->cnt++;
            return miss; /* compulsory miss */
        } else {
            rmv_head(set);
            return eviction;
        }
    } else if (p->v == 0) {
        p->v = 1;
        p->size = ad->size;
        p->tag = ad->tag;
        p->block_i = ad->block_i;
        return miss;
    }
    return hit;
}

void load_result(enum res result)
{
    switch (result) {
    case miss:
        inc_res(miss);
        break;
    case eviction:
        inc_res(miss);
        inc_res(eviction);
        break;
    case hit:
        inc_res(hit);
        break;
    case err:
        break;
    }
}

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
    newt->v = 0;
    newt->size = 0;
    newt->tag = 0;
    newt->block_i = 0;
    newt->next = NULL;
    return newt;
}

void rmv_head(struct cache_set *q)
{
    if (q != NULL && q->head != NULL) {
        struct cache_line *newh = q->head->next;
        free(q->head);
        q->head = newh;
    }
}

void parse_args(int argc, char *argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1)
        switch(ch) {
        case 'h':
            useage(argv[0]);
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
            useage(argv[0]);
            break;
        }
    if (_op.s == 0 || _op.E == 0 || _op.b == 0)
        useage(argv[0]);
}

void useage(char *name)
{
    printf("Useage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    exit(1);
}
