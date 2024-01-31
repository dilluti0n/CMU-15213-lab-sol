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

void block8_simple(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, i1, j1, tmp;
    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 8)
            for (i1 = i; i1 < i + 8 && i1 < N; i1++)
                for (j1 = j; j1 < j + 8 && j1 < M; j1++) {
                    tmp = A[i1][j1];
                    B[j1][i1] = tmp;
                }

}

/* 32x32, 61x67 passed but 64x64 goes wrong.... */
void block8(int M, int N, int A[N][M], int B[M][N])
{
    register int i, j, i1, j1;
    int row[8];
    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 8) {
            for (i1 = i; i1 < i + 8 && i1 < N; i1++) {
                for (j1 = j; j1 < j + 8 && j1 < M; j1++)
                    row[j1-j] = A[i1][j1];
                for (j1 = j; j1 < j + 8 && j1 < M; j1++)
                    B[j1][i1] = row[j1-j];
            }
        }
}

void block8_for64(int M, int N, int A[N][M], int B[M][N])
{
    register int i, j, i1, j1;
    int row[8];
    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 4)
            for (i1 = i; i1 < i + 8 && i1 < N; i1++) {
                for (j1 = j; j1 < j + 4 && j1 < M; j1++)
                    row[j1-j] = A[i1][j1];
                for (j1 = j; j1 < j + 4 && j1 < M; j1++)
                    B[j1][i1] = row[j1-j];
            }
}

void avoid_eviction(int M, int N, int A[N][M], int B[M][N])
{
    register int i, j, i1, j1;
    int diagonal[4];
    for (i = 0; i < N; i += 4)
        for (j = 0; j < M; j += 4) {
            for (i1 = i; i1 < i + 4 && i1 < N; i1++) {
                for (j1 = j; j1 < j + 4 && j1 < M; j1++) {
                    if (i1-i == j1-j) {
                        diagonal[j1-j] = A[i1][j1];
                    } else {
                        B[j1][i1] = A[i1][j1];
                    }
                }
            }
            for (j1 = j; j1 < j + 4 && j1 < M; j1++)
                B[j1][j1-j+i] = diagonal[j1-j];
        }
}

/* 
 * only for 64x64 matrix 
 * store next block of B.
 */
void opfor64(int M, int N, int A[N][M], int B[M][N])
{
    register int i1, j1, i, j, tmp;
    for (i = 0; i < N; i += 16)
        for (j = 0; j < M; j += 4) {
            for (i1 = i; i1 < i + 8; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1+8] = A[i1][j1];
            for (; i1 < i + 16; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1-8] = A[i1][j1];
            for (j1 = j; j1 < j + 4; j1++)
                for (i1 = i; i1 < i + 8; i1++) {
                    tmp = B[j1][i1];
                    B[j1][i1] = B[j1][i1+8];
                    B[j1][i1+8] = tmp;
                }
        }
}

/* 
 * only for 64x64 matrix 
 * store next block of B.
 * This is also evicted.
 */
void opfor64_m(int M, int N, int A[N][M], int B[M][N])
{
    register int i1, j1, i, j, tmp;
    /* assume N is 64(or any multiple of 8 greater than 8.) */
    const int hN = N >> 1; 
    for (i = 0; i < hN; i += 8)
        for (j = 0; j < M; j += 4) {
            for (i1 = i; i1 < i + 8; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1-i+N-i-8] = A[i1][j1]; /* (i1-i)+(N-i-8) */
            for (i1 = N - i - 8; i1 < N - i; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1-N+i+8+i] = A[i1][j1]; /* (i1-(N-i-8))+i */
            for (j1 = j; j1 < j + 4; j1++)
                for (i1 = i; i1 < i + 8; i1++) {
                    tmp = B[j1][i1];
                    B[j1][i1] = B[j1][i1-i+N-i-8];
                    B[j1][i1-i+N-i-8] = tmp;
                }
        }
}

/* 
 * only for 64x64 matrix 
 * store next block of B. 
 * not working...(why?)
 */
void opfor64_f(int M, int N, int A[N][M], int B[M][N])
{
    register int i1, j1, i, j, tmp;
    for (i = 0; i < N; i += 16)
        for (j = 0; j < M - 8; j += 4) { /* for j1 = 0 ~ M-9 */
            for (i1 = i; i1 < i + 8; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1+8] = A[i1][j1];
            for (; i1 < i + 16; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1+8][i1] = A[i1][j1]; /* store under */
            for (j1 = j; j1 < j + 4; j1++)
                for (i1 = i; i1 < i + 8; i1++)
                    B[j1][i1] = B[j1+8][i1+8];
            for (j1 = j; j1 < j + 4; j1++)
                for (i1 = i; i1 < i + 8; i1++) {
                    tmp = B[j1][i1];
                    B[j1][i1] = B[j1][i1+8];
                    B[j1][i1+8] = tmp;
                }
        }
    for (i = 0; i < N; i += 16)
        for (; j < M; j += 4) { /* for j1 = M-8 ~ M-1*/
            for (i1 = i; i1 < i + 8; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1+8] = A[i1][j1];
            for (; i1 < i + 16; i1++)
                for (j1 = j; j1 < j + 4; j1++)
                    B[j1][i1-8] = A[i1][j1];
            for (j1 = j; j1 < j + 4; j1++)
                for (i1 = i; i1 < i + 8; i1++) {
                    tmp = B[j1][i1];
                    B[j1][i1] = B[j1][i1+8];
                    B[j1][i1+8] = tmp;
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

    //registerTransFunction(block8_simple, "simple");  
    //registerTransFunction(block8, "block8");
    //registerTransFunction(block8_for64, "block8 for 64x64 matrix");
    //registerTransFunction(avoid_eviction, "avoid eviction caused in square matrix");
    //registerTransFunction(opfor64, "simple");
    registerTransFunction(opfor64_m, "FOR 64x64 matrix. mirror");
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

