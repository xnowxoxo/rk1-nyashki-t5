// Последовательная версия: эталон результата и время T1.
// Сборка: gcc -O3 seq.c -o seq -lm
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mandel.h"

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(void) {
    int n = mandel_size();
    int *img = malloc((size_t)n * n * sizeof(int));
    if (!img) { fprintf(stderr, "malloc failed\n"); return 1; }

    double t0 = now();                       // замеряем только вычислительное ядро
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++)
            img[(long)y * n + x] = mandel_pixel(x, y, n);
    double t = now() - t0;

    uint64_t sum = mandel_checksum(img, n);
    // seq — эталон: при первом запуске записывает ref.txt, дальше сверяется с ним
    printf("seq;1;%.6f;%s\n", t, mandel_check(sum, 1));
    printf("#checksum;%llu\n", (unsigned long long)sum);

    // По желанию сохраняем картинку для слайда: MANDEL_PGM=mandel.pgm ./seq
    const char *pgm = getenv("MANDEL_PGM");
    if (pgm) {
        FILE *f = fopen(pgm, "wb");
        if (f) {
            fprintf(f, "P5\n%d %d\n255\n", n, n);
            for (long i = 0; i < (long)n * n; i++) {
                int v = img[i] >= MAXIT ? 0 : 255 - (img[i] * 8 % 256);
                fputc(v, f);
            }
            fclose(f);
        }
    }
    free(img);
    return 0;
}
