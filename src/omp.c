// OpenMP-версия: строки картинки делятся между потоками, расписание задаётся
// снаружи через OMP_SCHEDULE (schedule(runtime)), поэтому static / dynamic / guided
// и размер порции сравниваются без перекомпиляции.
// Сборка: gcc -O3 -fopenmp omp.c -o omp -lm
//
// Почему расписание важно: тяжёлые строки (через середину множества) идут подряд.
// При static без chunk они достаются нескольким средним потокам, остальные ждут.
// static,1 и dynamic,1 раздают строки вперемешку, и нагрузка выравнивается.
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "mandel.h"

int main(void) {
    int n = mandel_size();
    int *img = malloc((size_t)n * n * sizeof(int));
    int nt = omp_get_max_threads();
    uint64_t *work = calloc(nt, sizeof(uint64_t));   // итераций на поток — для дисбаланса
    if (!img || !work) { fprintf(stderr, "malloc failed\n"); return 1; }

    double t0 = omp_get_wtime();                      // замеряем только вычислительное ядро
    #pragma omp parallel
    {
        uint64_t my = 0;                              // своя сумма у каждого потока, без гонок
        #pragma omp for schedule(runtime)
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++) {
                int it = mandel_pixel(x, y, n);
                img[(long)y * n + x] = it;
                my += it;
            }
        work[omp_get_thread_num()] = my;              // одна запись на поток после цикла
    }
    double t = omp_get_wtime() - t0;

    const char *sched = getenv("OMP_SCHEDULE");
    if (!sched) sched = "default";
    printf("omp_%s;%d;%.6f;%s\n", sched, nt, t, mandel_check(mandel_checksum(img, n), 0));

    // Строка для анализа: работа самого загруженного и самого свободного потока.
    // imbalance = max / среднее (1.0 — идеальный баланс)
    uint64_t wmin = work[0], wmax = work[0], wsum = 0;
    for (int i = 0; i < nt; i++) {
        if (work[i] < wmin) wmin = work[i];
        if (work[i] > wmax) wmax = work[i];
        wsum += work[i];
    }
    printf("#detail;omp_%s;%d;work_min=%llu;work_max=%llu;imbalance=%.4f\n", sched, nt,
           (unsigned long long)wmin, (unsigned long long)wmax,
           wsum ? (double)wmax * nt / wsum : 1.0);

    free(work);
    free(img);
    return 0;
}
