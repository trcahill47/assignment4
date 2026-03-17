/*
 * pth_matrix_matrix.c
 * Pthreads parallel O(N^3) matrix-matrix multiplication
 * Usage: ./pth_matrix_matrix <file A> <file B> <file C> <threads>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "quinn_defs.h"

static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Shared globals — read-only after setup, so no mutex needed */
int M, K, N;
int thread_count;
double *A, *B, *C;

void *pth_multiply(void *rank) {
    long my_rank   = (long) rank;
    int row_start  = BLOCK_LOW(my_rank,  thread_count, M);
    int row_end    = BLOCK_HIGH(my_rank, thread_count, M);

    for (int m = row_start; m <= row_end; m++) {
        for (int k = 0; k < K; k++) {
            double a_mk = A[m * K + k];
            for (int r = 0; r < N; r++) {
                C[m * N + r] += a_mk * B[k * N + r];
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file A> <file B> <file C> <threads>\n", argv[0]);
        return 1;
    }
    thread_count = (int) strtol(argv[4], NULL, 10);

    double t_overall_start = get_time();

    /* --- Read A --- */
    double t_io_in_start = get_time();
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror("Cannot open file A"); return 1; }
    int A_rows, A_cols;
    fread(&A_rows, sizeof(int), 1, fp);
    fread(&A_cols, sizeof(int), 1, fp);
    A = malloc(A_rows * A_cols * sizeof(double));
    fread(A, sizeof(double), A_rows * A_cols, fp);
    fclose(fp);

    /* --- Read B --- */
    fp = fopen(argv[2], "rb");
    if (!fp) { perror("Cannot open file B"); return 1; }
    int B_rows, B_cols;
    fread(&B_rows, sizeof(int), 1, fp);
    fread(&B_cols, sizeof(int), 1, fp);
    B = malloc(B_rows * B_cols * sizeof(double));
    fread(B, sizeof(double), B_rows * B_cols, fp);
    fclose(fp);
    double t_io_in_end = get_time();

    if (A_cols != B_rows) {
        fprintf(stderr, "Error: A is %dx%d but B is %dx%d — inner dims must match.\n",
                A_rows, A_cols, B_rows, B_cols);
        return 1;
    }

    M = A_rows; K = A_cols; N = B_cols;
    C = calloc(M * N, sizeof(double));
    if (!A || !B || !C) { fprintf(stderr, "malloc failed\n"); return 1; }

    printf("Pthreads Matrix-Matrix: (%d x %d) x (%d x %d) -> (%d x %d) P=%d\n",
           M, K, K, N, M, N, thread_count);

    /* --- Spawn threads --- */
    pthread_t *handles = malloc(thread_count * sizeof(pthread_t));
    double t_thread_create_start = get_time();
    for (long t = 0; t < thread_count; t++)
        pthread_create(&handles[t], NULL, pth_multiply, (void *) t);

    double t_work_start = get_time();
    for (long t = 0; t < thread_count; t++)
        pthread_join(handles[t], NULL);
    double t_work_end = get_time();
    double t_thread_create_end = t_work_start;   /* creation finished before join */

    /* --- Write C --- */
    double t_io_out_start = get_time();
    fp = fopen(argv[3], "wb");
    if (!fp) { perror("Cannot open file C"); return 1; }
    fwrite(&M, sizeof(int), 1, fp);
    fwrite(&N, sizeof(int), 1, fp);
    fwrite(C, sizeof(double), M * N, fp);
    fclose(fp);
    double t_io_out_end  = get_time();
    double t_overall_end = get_time();

    double t_io_in        = t_io_in_end        - t_io_in_start;
    double t_thread_create= t_thread_create_end- t_thread_create_start;
    double t_work         = t_work_end         - t_work_start;
    double t_io_out       = t_io_out_end       - t_io_out_start;
    double t_overall      = t_overall_end      - t_overall_start;

    printf("--------------------------------------\n");
    printf("  IO Read       time : %.6f s\n", t_io_in);
    printf("  Thread create time : %.6f s\n", t_thread_create);
    printf("  Work          time : %.6f s\n", t_work);
    printf("  IO Write      time : %.6f s\n", t_io_out);
    printf("  Overall       time : %.6f s\n", t_overall);
    printf("--------------------------------------\n");
    /* CSV: version, M, K, N, P, t_overall, t_work, t_io_in, t_io_out, t_thread_create */
    printf("CSV,pthreads,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f\n",
           M, K, N, thread_count, t_overall, t_work, t_io_in, t_io_out, t_thread_create);

    free(A); free(B); free(C); free(handles);
    return 0;
}
