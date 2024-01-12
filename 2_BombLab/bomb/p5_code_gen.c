#include <stdio.h>

/*
 * Useage: gcc p4_code_gen.c -o p4lmacro
 * ./p4lmacro >> defuse_code.txt
 * make sure there is '\n' in the end of defuse_code.txt
 */

int main()
{
    putc(9, stdout);
    putc(15, stdout);
    putc(14, stdout);
    putc(5, stdout);
    putc(6, stdout);
    putc(7, stdout);
    putc('\n', stdout);
    return 0;
}