// Track 11: histogram + counting sort of 5*10^8 integers in [0, 256). OpenMP: atomic vs local histograms.
// Build: gcc -O3 -fopenmp omp.c -o omp
// Run:   OMP_NUM_THREADS=8 ./omp atomic|local
// Output: version;p;time_s;CHECK, then a '#' line with the histogram and fill phase times.
// Timed kernel = histogram + prefix sum + fill of the sorted array. Data generation and CHECK are not timed.

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hist.h"

static long long hist[K], ref[K], cnt[K];
static long start[K + 1];

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "";
    const int atom = !strcmp(mode, "atomic");
    if (!atom && strcmp(mode, "local")) {
        fprintf(stderr, "usage: %s atomic | local\n", argv[0]);
        return 1;
    }
    int *a = malloc(N * sizeof *a), *out = malloc(N * sizeof *out);
    if (!a || !out) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    // First touch with the same static split as the kernel, so each thread's pages sit on its own NUMA node.
#pragma omp parallel for schedule(static)
    for (long i = 0; i < N; i++) {
        a[i] = value(i);
        out[i] = -1;  // not 0: gcc turns malloc + zero fill into calloc, leaving the pages untouched
    }

    double t0 = omp_get_wtime();
    if (atom) {
        // One shared histogram: every increment is a locked read-modify-write on a cache line all threads fight over.
#pragma omp parallel for schedule(static)
        for (long i = 0; i < N; i++) {
#pragma omp atomic
            hist[a[i]]++;
        }
    } else {
        // Local histograms: each thread counts its block into its own h with the same count() as seq.c,
        // then adds h to the shared hist once, so the hot loop has no atomics and no shared writes.
#pragma omp parallel
        {
            long long h[K] = {0};
            const int nt = omp_get_num_threads(), t = omp_get_thread_num();
            const long q = (N + nt - 1) / nt;  // the same blocks as schedule(static) above
            const long lo = t * q < N ? t * q : N, hi = lo + q < N ? lo + q : N;
            count(a, lo, hi, h);
#pragma omp critical
            for (int b = 0; b < K; b++) hist[b] += h[b];
        }
    }
    double t1 = omp_get_wtime();

    // Counting sort: value b fills out[start[b] .. start[b+1]). K = 256, so the prefix sum stays sequential.
    for (int b = 0; b < K; b++) start[b + 1] = start[b] + hist[b];
#pragma omp parallel for schedule(static)
    for (int b = 0; b < K; b++)
        for (long j = start[b]; j < start[b + 1]; j++) out[j] = b;
    double t2 = omp_get_wtime();

    // CHECK: recount the input sequentially; hist must match, out must be sorted with the same counts.
    for (long i = 0; i < N; i++) ref[a[i]]++;
    int ok = 1;
    for (long i = 0; i < N; i++) {
        cnt[out[i]]++;
        if (i && out[i - 1] > out[i]) ok = 0;
    }
    for (int b = 0; b < K; b++) ok &= hist[b] == ref[b] && cnt[b] == ref[b];

    printf("%s;%d;%.4f;CHECK: %s\n", atom ? "omp-atomic" : "omp-local", omp_get_max_threads(), t2 - t0,
           ok ? "OK" : "FAIL");
    printf("# hist=%.4f fill=%.4f\n", t1 - t0, t2 - t1);
    free(a);
    free(out);
    return !ok;
}
