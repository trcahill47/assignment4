/*
 * matrix_matrix.c
 * Serial O(N^3) matrix-matrix multiplication
 * Usage: ./matrix_matrix <file A> <file B> <file C>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* Helper to get wall time in seconds */
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file A> <file B> <file C>\n", argv[0]);
        return 1;
    }

    double t_overall_start = get_time();

    /* ------------------------------------------------------------------ */
    /* I/O - Read A                                                         */
    /* ------------------------------------------------------------------ */
    double t_io_in_start = get_time();

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) { perror("Cannot open file A"); return 1; }

    int A_rows, A_cols;
    fread(&A_rows, sizeof(int), 1, fp);
    fread(&A_cols, sizeof(int), 1, fp);

    double* A = malloc(A_rows * A_cols * sizeof(double));
    if (!A) { fprintf(stderr, "malloc failed for A\n"); return 1; }
    fread(A, sizeof(double), A_rows * A_cols, fp);
    fclose(fp);

    /* ------------------------------------------------------------------ */
    /* I/O - Read B                                                         */
    /* ------------------------------------------------------------------ */
    fp = fopen(argv[2], "rb");
    if (!fp) { perror("Cannot open file B"); return 1; }

    int B_rows, B_cols;
    fread(&B_rows, sizeof(int), 1, fp);
    fread(&B_cols, sizeof(int), 1, fp);

    double* B = malloc(B_rows * B_cols * sizeof(double));
    if (!B) { fprintf(stderr, "malloc failed for B\n"); return 1; }
    fread(B, sizeof(double), B_rows * B_cols, fp);
    fclose(fp);

    double t_io_in_end = get_time();

    /* ------------------------------------------------------------------ */
    /* Dimension check                                                      */
    /* ------------------------------------------------------------------ */
    if (A_cols != B_rows) {
        fprintf(stderr,
            "Error: A is %dx%d but B is %dx%d - inner dimensions must match.\n",
            A_rows, A_cols, B_rows, B_cols);
        return 1;
    }

    int M = A_rows;   /* rows of C    */
    int K = A_cols;   /* inner dim    */
    int N = B_cols;   /* cols of C    */

    printf("Serial Matrix-Matrix Multiply: (%d x %d) x (%d x %d) -> (%d x %d)\n",
           M, K, K, N, M, N);

    double* C = calloc(M * N, sizeof(double));
    if (!C) { fprintf(stderr, "malloc failed for C\n"); return 1; }

    /* ------------------------------------------------------------------ */
    /* Compute - naive O(N^3) triple loop (m, k, r order)                  */
    /*                                                                      */
    /* Loop order is m -> k -> r (not m -> r -> k).                        */
    /* In the inner loop r increments, so B[k][r] steps across a row of B  */
    /* which is sequential in memory -- cache friendly.                     */
    /* ------------------------------------------------------------------ */
    double t_work_start = get_time();

    for (int m = 0; m < M; m++) {
        for (int k = 0; k < K; k++) {
            double a_mk = A[m * K + k];          /* load once, reuse N times */
            for (int r = 0; r < N; r++) {
                C[m * N + r] += a_mk * B[k * N + r];
            }
        }
    }

    double t_work_end = get_time();

    /* ------------------------------------------------------------------ */
    /* I/O - Write C                                                        */
    /* ------------------------------------------------------------------ */
    double t_io_out_start = get_time();

    fp = fopen(argv[3], "wb");
    if (!fp) { perror("Cannot open file C"); return 1; }
    fwrite(&M, sizeof(int), 1, fp);
    fwrite(&N, sizeof(int), 1, fp);
    fwrite(C, sizeof(double), M * N, fp);
    fclose(fp);

    double t_io_out_end  = get_time();
    double t_overall_end = get_time();

    /* ------------------------------------------------------------------ */
    /* Timing report                                                        */
    /* ------------------------------------------------------------------ */
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

    /* Machine-readable CSV line for batch scripts:                        */
    /* version, M, K, N, P, t_overall, t_work, t_io_in, t_io_out          */
    printf("CSV,serial,%d,%d,%d,1,%.6f,%.6f,%.6f,%.6f\n",
           M, K, N, t_overall, t_work, t_io_in, t_io_out);

    free(A);
    free(B);
    free(C);
    return 0;
}
