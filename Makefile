# Собирать на bilim-master:  module load intel/mpi/2021.14 && make
CC     ?= gcc
MPICC  ?= mpicc
CFLAGS ?= -O3 -Wall

all: bin/seq bin/omp bin/mpi

bin:
	mkdir -p bin

bin/seq: src/seq.c src/mandel.h | bin
	$(CC) $(CFLAGS) src/seq.c -o $@ -lm

bin/omp: src/omp.c src/mandel.h | bin
	$(CC) $(CFLAGS) -fopenmp src/omp.c -o $@ -lm

bin/mpi: src/mpi.c src/mandel.h | bin
	$(MPICC) $(CFLAGS) src/mpi.c -o $@ -lm

clean:
	rm -rf bin

.PHONY: all clean
