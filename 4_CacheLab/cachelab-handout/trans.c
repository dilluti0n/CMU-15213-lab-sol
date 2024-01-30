/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

char trans_block4_desc[] = "simple cache-optimized version with 4 blocks";
void trans_block4(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, i1, j1, tmp;
    const int S = 4;
    for (i = 0; i < N; i += S)
        for (j = 0; j < M; j += S)
            for (i1 = i; i1 < i + S && i1 < N; i1++)
                for (j1 = j; j1 < j + S && j1 < M; j1++) {
                    tmp = A[i1][j1];
                    B[j1][i1] = tmp;
                }

}

char trans_block_desc[] = "cache-optimized version with BLOCK 4";
void trans_block(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, i1, j1;
    int a, b, c, d;
    const int S = 4;
    for (i = 0; i < N; i += S)
        for (j = 0; j < M; j += S) {
            i1 = i;
            if (j + S < M) {
                a = A[i1][j];
                b = A[i1][j+1];
                c = A[i1][j+2];
                d = A[i1][j+3];
                B[j][i1] = a;
                B[j+1][i1] = b;
                B[j+2][i1] = c;
                B[j+3][i1] = d;
                i1++;
            }
            for (; i1 < i + S && i1 < N; i1++) {
                for (j1 = j; j1 < j + S && j1 < M; j1++) {
                    a = A[i1][j1];
                    B[j1][i1] = a;
                }
            }
        }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

    registerTransFunction(trans_block4, trans_block4_desc); 
    registerTransFunction(trans_block, trans_block_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

