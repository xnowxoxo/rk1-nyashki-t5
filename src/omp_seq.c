// Track 11: histogram + counting sort of 5*10^8 integers in [0, 256). Sequential reference (T1).
// Build: gcc -O3 seq.c -o seq
// Run:   ./seq
// Output: seq;1;time_s;CHECK, then a '#' line with the histogram and fill phase times.
// Timed kernel = histogram + prefix sum + fill of the sorted array. Data generation and CHECK are not timed.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hist.h"

static long long hist[K], ref[K], cnt[K];
static long start[K + 1];

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void) {
    int *a = malloc(N * sizeof *a), *out = malloc(N * sizeof *out);
    if (!a || !out) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    for (long i = 0; i < N; i++) {
        a[i] = value(i);
        out[i] = -1;  // not 0: gcc turns malloc + zero fill into calloc, leaving the pages untouched
    }

    double t0 = now();
    count(a, 0, N, hist);
    double t1 = now();

    // Counting sort: value b fills out[start[b] .. start[b+1]).
    for (int b = 0; b < K; b++) start[b + 1] = start[b] + hist[b];
    for (int b = 0; b < K; b++)
        for (long j = start[b]; j < start[b + 1]; j++) out[j] = b;
    double t2 = now();

    // CHECK: recount the input; hist must match, out must be sorted with the same counts.
    for (long i = 0; i < N; i++) ref[a[i]]++;
    int ok = 1;
    for (long i = 0; i < N; i++) {
        cnt[out[i]]++;
        if (i && out[i - 1] > out[i]) ok = 0;
    }
    for (int b = 0; b < K; b++) ok &= hist[b] == ref[b] && cnt[b] == ref[b];

    printf("seq;1;%.4f;CHECK: %s\n", t2 - t0, ok ? "OK" : "FAIL");
    printf("# hist=%.4f fill=%.4f\n", t1 - t0, t2 - t1);
    free(a);
    free(out);
    return !ok;
}
