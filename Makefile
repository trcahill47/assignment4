CC      = gcc
CFLAGS  = -O3 -Wall
OMPFLAGS= -fopenmp
LDFLAGS = -lpthread

all: make_matrix print_matrix matrix_matrix pth_matrix_matrix omp_matrix_matrix

make_matrix: make_matrix.c
	$(CC) $(CFLAGS) -o $@ $<

print_matrix: print_matrix.c
	$(CC) $(CFLAGS) -o $@ $<

matrix_matrix: matrix_matrix.c
	$(CC) $(CFLAGS) -o $@ $<

pth_matrix_matrix: pth_matrix_matrix.c quinn_defs.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

omp_matrix_matrix: omp_matrix_matrix.c
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $<

clean:
	rm -f make_matrix print_matrix matrix_matrix pth_matrix_matrix omp_matrix_matrix *.o
