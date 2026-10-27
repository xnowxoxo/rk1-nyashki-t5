// Shared by seq.c and omp.c: the input data and the counting kernel, so T1 and T(p) time the same machine code.
#include <stdint.h>

#define N 500000000L
#define K 256

// splitmix64 of the index: the same data for any thread count, uniform over [0, K).
static int value(uint64_t i) {
    uint64_t z = (i + 1) * 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return (int)((z ^ (z >> 31)) >> 56);
}

// Histogram of a[lo, hi) into h. noinline: one copy of the loop, whoever calls it.
__attribute__((noinline)) static void count(const int *a, long lo, long hi, long long *h) {
    for (long i = lo; i < hi; i++) h[a[i]]++;
}
