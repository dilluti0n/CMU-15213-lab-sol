#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

struct {
    int verbose;
    int s;
    int E;
    int b;
    FILE *fp;
} option; /* inline argument options */

struct cache_line {
    int v : 1;
    int tag;
    int size;
    int block;
    struct cache_line *next;
};

struct cache_set {
    int cnt;
    struct cache_line *head;
    struct cache_line *tail;
} *S;

enum cache_search_res {
    miss = 0,
    hit,
    eviction,
};

void parse_args(int argc, char *argv[]);
void useage(char *name);

int main(int argc, char *argv[])
{
    option.fp = stdin;
    parse_args(argc, argv);
    if ((S = malloc(1 << option.s)) == NULL) /* S[2^s] */
        exit(3);
    free(S);
    exit(0);
}

int add_tail(struct cache_set *q) {
    if (q == NULL) {
        return -1;
    } else if (q->head == NULL) {
        if ((q->head = malloc(sizeof(struct cache_line))) == NULL)
            return -1;
        q->tail = q->head;
    } else {
        struct cache_line *newt;
        if ((newt = malloc(sizeof(struct cache_line)) == NULL))
            return -1;
        newt->next = NULL;
        q->tail->next = newt;
        q->tail = newt;
    }
    return 0;
}

int rmv_head(struct cache_set *q) {
    if (q == NULL || q->head == NULL) {
        return -1;
    } else {
        struct cache_line *newh = q->head->next;
        free(q->head);
        q->head = newh;
    }
    return 0;
}

int parse_input() {
    char id;
    unsigned adress;
    int size;
    while (fscanf(option.fp, " %c %x, %d", &id, &adress, &size) > 0) {

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
            option.verbose = 1;
            break;
        case 's':
            option.s = atoi(optarg);
            break;
        case 'E':
            option.E = atoi(optarg);
            break;
        case 'b':
            option.b = atoi(optarg);
            break;
        case 't':
            if ((option.fp = fopen(optarg, "r")) == NULL) {
                fprintf(stderr, "err: invalid path %s\n", optarg);
                exit(2);
            }
            break;
        case '?':
        default:
            fprintf(stderr, "err: invalid argument %s\n", optopt);
            useage(argv[0]);
            break;
        }
    if (option.s == 0 || option.E == 0 || option.b == 0)
        usage(argv[0]);
}

void useage(char *name)
{
    fprintf(stderr, "Useage: ./%s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    exit(1);
}
