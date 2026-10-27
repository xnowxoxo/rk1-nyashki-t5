// Track 5: Mandelbrot set, 4096 x 4096, up to 2000 iterations. Sequential reference (T1).
// Build: gcc -O3 seq.c -o seq -lm
// Run:   ./seq
// Output: seq;1;time_s;CHECK, then a '#' line with the image hash that every omp run must reproduce.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 4096
#define MAXIT 2000
#define XMIN (-2.0)
#define YMIN (-1.5)
#define D (3.0 / N)  // pixel size: the region is [-2, 1] x [-1.5, 1.5]

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Computes one image row. Same kernel as in omp.c.
static void row(int *img, int y) {
    const double ci = YMIN + (y + 0.5) * D;
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
    }
}

// FNV-1a over all pixels in order: a wrong value or a misplaced row changes it.
static uint64_t hash(const int *img) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t k = 0; k < (size_t)N * N; k++) h = (h ^ (uint64_t)img[k]) * 1099511628211ULL;
    return h;
}

// Row y and row N-1-y have exactly opposite ci, so a correct image is mirror-symmetric bit for bit.
static int symmetric(const int *img) {
    for (int y = 0; y < N / 2; y++)
        if (memcmp(img + (size_t)y * N, img + (size_t)(N - 1 - y) * N, N * sizeof *img)) return 0;
    return 1;
}

int main(void) {
    int *img = malloc((size_t)N * N * sizeof *img);
    if (!img) return 1;

    double t = now();
    for (int y = 0; y < N; y++) row(img, y);
    t = now() - t;

    const int ok = symmetric(img);
    printf("seq;1;%.4f;CHECK: %s\n", t, ok ? "OK" : "FAIL");
    printf("# hash=%llu\n", (unsigned long long)hash(img));
    free(img);
    return !ok;
}
