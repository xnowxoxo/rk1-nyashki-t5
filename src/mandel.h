// Общие параметры и вычисление одного пикселя — одинаково для seq, omp и mpi,
// поэтому результат во всех версиях совпадает побитово.
#ifndef MANDEL_H
#define MANDEL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXIT 2000
#define XMIN (-2.0)
#define XMAX ( 1.0)
#define YMIN (-1.5)
#define YMAX ( 1.5)

// Размер картинки N x N. По умолчанию 4096 (по заданию),
// для быстрой проверки можно задать переменную окружения MANDEL_N.
static inline int mandel_size(void) {
    const char *s = getenv("MANDEL_N");
    return s ? atoi(s) : 4096;
}

// Число итераций до выхода |z| > 2 (не больше MAXIT)
static inline int mandel_pixel(int x, int y, int n) {
    double cr = XMIN + (XMAX - XMIN) * x / (n - 1);
    double ci = YMIN + (YMAX - YMIN) * y / (n - 1);
    double zr = 0.0, zi = 0.0;
    int it = 0;
    while (it < MAXIT && zr * zr + zi * zi <= 4.0) {
        double t = zr * zr - zi * zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = t;
        it++;
    }
    return it;
}

// Контрольная сумма с весом позиции: ловит не только неверные значения,
// но и строки, собранные не в том порядке (важно для циклической раздачи в MPI).
static inline uint64_t mandel_checksum(const int *img, int n) {
    uint64_t s = 0;
    for (long i = 0; i < (long)n * n; i++)
        s += (uint64_t)img[i] * (uint64_t)(i + 1);
    return s;
}

// Проверка корректности по эталонной сумме из файла (по умолчанию ref_<N>.txt,
// путь можно задать через REF_FILE). Эталон пишет seq при первом запуске.
//   is_ref = 1 (seq): файла нет -> записать его и вернуть OK; есть -> сверить.
//   is_ref = 0 (omp, mpi): файла нет -> NOREF; есть -> OK или FAIL.
static inline const char *mandel_check(uint64_t sum, int is_ref) {
    // эталон свой для каждого размера: ref_4096.txt, ref_512.txt, ...
    char defpath[64];
    snprintf(defpath, sizeof defpath, "ref_%d.txt", mandel_size());
    const char *path = getenv("REF_FILE");
    if (!path) path = defpath;
    FILE *f = fopen(path, "r");
    if (!f) {
        if (!is_ref) return "NOREF";
        f = fopen(path, "w");
        if (!f) return "NOREF";
        fprintf(f, "%llu\n", (unsigned long long)sum);
        fclose(f);
        return "OK";
    }
    unsigned long long ref = 0;
    int ok = fscanf(f, "%llu", &ref) == 1;
    fclose(f);
    return (ok && ref == sum) ? "OK" : "FAIL";
}

#endif
