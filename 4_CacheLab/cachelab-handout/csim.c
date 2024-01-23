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
} option;

FILE *tracefile;

int parse_args(int argc, char *argv[]);
void useage(char *name);

int main(int argc, char *argv[])
{
    printSummary(0, 0, 0);
    return 0;
}

int parse_args(int argc, char *argv[])
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
            tracefile = fopen(optarg, "r");
            break;
        case '?':
        default:
            useage(argv[0]);
            break;
        }
    return ch;
}

void useage(char *name)
{
    fprintf(stderr, "Useage: ./%s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    exit(1);
}