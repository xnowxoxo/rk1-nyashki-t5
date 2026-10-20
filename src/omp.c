// OpenMP-версия — пишет OpenMP-инженер команды.
// Сборка: gcc -O3 -fopenmp omp.c -o omp -lm
//
// Что должно быть по треку 5:
//   - параллельный цикл по строкам y;
//   - schedule(runtime), чтобы сравнить static / dynamic / guided
//     без перекомпиляции (omp.sbatch задаёт OMP_SCHEDULE);
//   - время только вычислительного ядра (omp_get_wtime вокруг цикла);
//   - вывод строго в формате: omp_<schedule>;<потоки>;<время>;<CHECK>
//     CHECK берётся из mandel_check(sum, 0) — сверка с эталоном seq.
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "mandel.h"

int main(void) {
    int n = mandel_size();
    int *img = malloc((size_t)n * n * sizeof(int));

    double t0 = omp_get_wtime();
    // TODO: распараллелить цикл ниже (#pragma omp parallel for schedule(runtime))
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++)
            img[(long)y * n + x] = mandel_pixel(x, y, n);
    double t = omp_get_wtime() - t0;

    const char *sched = getenv("OMP_SCHEDULE");
    printf("omp_%s;%d;%.6f;%s\n", sched ? sched : "default",
           omp_get_max_threads(), t, mandel_check(mandel_checksum(img, n), 0));
    free(img);
    return 0;
}
