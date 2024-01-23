#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

struct option {
    int verbose;
    int s;
    int E;
    int b;
    FILE *file;
};

void parse_args(int argc, char *argv[], struct option *op);
void useage(char *name);

int main(int argc, char *argv[])
{
    struct option op = {
        0,
        0,
        0,
        0,
        stdin
    };
    parse_args(argc, argv, &op);
    exit(0);
}

void parse_args(int argc, char *argv[], struct option *op)
{
    int ch;
    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1)
        switch(ch) {
        case 'h':
            useage(argv[0]);
            break;
        case 'v':
            op->verbose = 1;
            break;
        case 's':
            op->s = atoi(optarg);
            break;
        case 'E':
            op->E = atoi(optarg);
            break;
        case 'b':
            op->b = atoi(optarg);
            break;
        case 't':
            op->file = fopen(optarg, "r");
            break;
        case '?':
        default:
            fprintf(stderr, "err: invalid argument %s\n", optopt);
            useage(argv[0]);
            break;
        }
    if (op->s == 0 || op->E == 0 || op->b == 0)
        usage(argv[0]);
}

void useage(char *name)
{
    fprintf(stderr, "Useage: ./%s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    exit(1);
}
