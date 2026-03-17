/* make_matrix.c

 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <filename> <rows> <cols>\n", argv[0]);
    return 1;

  }

  char* filename = argv[1];
  int m = atoi(argv[2]);
  int n = atoi(argv[3]);

  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) {
    perror("Cannot open file");
    return 1;
  }
	  
  fwrite(&m, sizeof(int), 1, fp);
  fwrite(&n, sizeof(int), 1, fp);

  srand(time(NULL));
  double* data = malloc(m * n * sizeof(double));
  for (int i = 0; i < m * n; i++) {
    data[i] = (double)rand() / RAND_MAX * 100.0;

  }
  fwrite(data, sizeof(double), m * n, fp);

  printf("Generated %s: %d x %d matrix.\n", filename, m, n);

  free(data);
  fclose(fp);
  return 0;
}
