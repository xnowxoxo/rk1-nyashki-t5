// Track 5: Mandelbrot set, 4096 x 4096, up to 2000 iterations. OpenMP: static / dynamic / guided.
// Build: gcc -O3 -fopenmp omp.c -o omp -lm
// Run:   OMP_SCHEDULE=dynamic,1 OMP_NUM_THREADS=8 ./omp <hash>   <hash> is printed by ./seq (the reference)
// Output: version;p;time_s;CHECK, then a '#' line with the image hash and the load imbalance.

#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N 4096
#define MAXIT 2000
#define XMIN (-2.0)
#define YMIN (-1.5)
#define D (3.0 / N)  // pixel size: the region is [-2, 1] x [-1.5, 1.5]

// Computes one image row, returns the iterations spent on it (the work, for load imbalance).
static uint64_t row(int *img, int y) {
    const double ci = YMIN + (y + 0.5) * D;
    uint64_t work = 0;
    for (int x = 0; x < N; x++) {
        const double cr = XMIN + (x + 0.5) * D;
        double zr = 0, zi = 0, zr2 = 0, zi2 = 0;
        int it = 0;
        while (it < MAXIT && zr2 + zi2 <= 4.0) {
            zi = 2.0 * zr * zi + ci;
            zr = zr2 - zi2 + cr;
            zr2 = zr * zr;
            zi2 = zi * zi;
            it++;
        }
        img[(size_t)y * N + x] = it;
        work += it;
    }
    return work;
}

// FNV-1a over all pixels in order: a wrong value or a misplaced row changes it.
static uint64_t hash(const int *img) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t k = 0; k < (size_t)N * N; k++) h = (h ^ (uint64_t)img[k]) * 1099511628211ULL;
    return h;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <reference hash from ./seq>\n", argv[0]);
        return 1;
    }
    const int p = omp_get_max_threads();
    int *img = malloc((size_t)N * N * sizeof *img);
    uint64_t work[256] = {0};
    if (!img || p > 256) return 1;

    double t = omp_get_wtime();
#pragma omp parallel
    {
        uint64_t local = 0;
#pragma omp for schedule(runtime)  // static / static,1 / dynamic,1 / guided, taken from OMP_SCHEDULE
        for (int y = 0; y < N; y++) local += row(img, y);
        work[omp_get_thread_num()] = local;  // one write per thread, no race
    }
    t = omp_get_wtime() - t;

    // must match the reference hash exactly
    const uint64_t h = hash(img);
    const int ok = h == strtoull(argv[1], NULL, 10);

    // imbalance = busiest thread's work / average work (1.0 = perfect balance)
    uint64_t mx = 0, sum = 0;
    for (int i = 0; i < p; i++) {
        sum += work[i];
        if (work[i] > mx) mx = work[i];
    }
    const char *sched = getenv("OMP_SCHEDULE");
    char ver[64];
    snprintf(ver, sizeof ver, "omp-%s", sched ? sched : "default");

    printf("%s;%d;%.4f;CHECK: %s\n", ver, p, t, ok ? "OK" : "FAIL");
    printf("# hash=%llu imbalance=%.3f\n", (unsigned long long)h, (double)mx * p / sum);
    free(img);
    return !ok;
}
