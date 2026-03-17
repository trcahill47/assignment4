/*
 * omp_matrix_matrix.c
 * OpenMP parallel O(N^3) matrix-matrix multiplication
 * Usage: ./omp_matrix_matrix <file A> <file B> <file C> <threads>
 *
 * Parallelization strategy:
 *   #pragma omp parallel for on the outermost loop (rows of A).
 *   Each thread gets a contiguous block of rows (default static schedule).
 *   No race condition: each value of m writes to a unique row of C.
 *   A and B are read-only, so no synchronization is needed at all.
 *   Loop variable m is private automatically (it is the for-loop index).
 *   a_mk is declared inside the loop body, so it lives on each thread's
 *   stack and is private automatically -- no firstprivate/private needed.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
 
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
 
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file A> <file B> <file C> <threads>\n", argv[0]);
        return 1;
    }
 
    int thread_count = (int) strtol(argv[4], NULL, 10);
    omp_set_num_threads(thread_count);
 
    double t_overall_start = get_time();
 
    /* --- Read A --- */
    double t_io_in_start = get_time();
 
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror("Cannot open file A"); return 1; }
    int A_rows, A_cols;
    fread(&A_rows, sizeof(int), 1, fp);
    fread(&A_cols, sizeof(int), 1, fp);
    double *A = malloc(A_rows * A_cols * sizeof(double));
    if (!A) { fprintf(stderr, "malloc failed for A\n"); return 1; }
    fread(A, sizeof(double), A_rows * A_cols, fp);
    fclose(fp);
 
    /* --- Read B --- */
    fp = fopen(argv[2], "rb");
    if (!fp) { perror("Cannot open file B"); return 1; }
    int B_rows, B_cols;
    fread(&B_rows, sizeof(int), 1, fp);
    fread(&B_cols, sizeof(int), 1, fp);
    double *B = malloc(B_rows * B_cols * sizeof(double));
    if (!B) { fprintf(stderr, "malloc failed for B\n"); return 1; }
    fread(B, sizeof(double), B_rows * B_cols, fp);
    fclose(fp);
 
    double t_io_in_end = get_time();
 
    /* --- Dimension check --- */
    if (A_cols != B_rows) {
        fprintf(stderr, "Error: A is %dx%d but B is %dx%d -- inner dims must match.\n",
                A_rows, A_cols, B_rows, B_cols);
        return 1;
    }
 
    int M = A_rows, K = A_cols, N = B_cols;
 
    double *C = calloc(M * N, sizeof(double));
    if (!C) { fprintf(stderr, "malloc failed for C\n"); return 1; }
 
    printf("OpenMP Matrix-Matrix: (%d x %d) x (%d x %d) -> (%d x %d) P=%d\n",
           M, K, K, N, M, N, thread_count);
 
    /* --- Compute ---
     * #pragma omp parallel for is a combined directive: it opens a parallel
     * region AND distributes the for loop across threads in one line.
     * No schedule() clause needed -- the default is static, which splits
     * the M iterations into P equal contiguous chunks, exactly what we want
     * for this problem since every row does the same amount of work.
     */
    double t_work_start = get_time();
 
    #pragma omp parallel for
    for (int m = 0; m < M; m++) {
        for (int k = 0; k < K; k++) {
            double a_mk = A[m * K + k];   /* private: declared inside loop */
            for (int r = 0; r < N; r++) {
                C[m * N + r] += a_mk * B[k * N + r];
            }
        }
    }
 
    double t_work_end = get_time();
 
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
 
    /* --- Timing report --- */
    double t_io_in   = t_io_in_end   - t_io_in_start;
    double t_work    = t_work_end    - t_work_start;
    double t_io_out  = t_io_out_end  - t_io_out_start;
    double t_overall = t_overall_end - t_overall_start;
 
    printf("--------------------------------------\n");
    printf("  IO Read  time : %.6f s\n", t_io_in);
    printf("  Work     time : %.6f s\n", t_work);
    printf("  IO Write time : %.6f s\n", t_io_out);
    printf("  Overall  time : %.6f s\n", t_overall);
    printf("--------------------------------------\n");
 
    /* CSV: version, M, K, N, P, t_overall, t_work, t_io_in, t_io_out */
    printf("CSV,openmp,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f\n",
           M, K, N, thread_count, t_overall, t_work, t_io_in, t_io_out);
 
    free(A); free(B); free(C);
    return 0;
}