/* print_matrix.c


 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  FILE* fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    perror("Cant open file");
    return 1;
  }

  int m,n;
  fread(&m, sizeof(int), 1, fp);
  fread(&n, sizeof(int), 1, fp);

  printf("Matrix Dimensions: %d rows x %d cols\n", m, n);

  double* data = malloc(m * n * sizeof(double));
  fread(data, sizeof(double), m * n, fp);

  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      printf("%6.2f ", data[i * n + j]);
      
    }
    printf("\n");
  }

  free(data);
  fclose(fp);
  return 0;
}
