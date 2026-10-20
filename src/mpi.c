// MPI-версия: строки раздаются циклически (rank r считает строки r, r+p, r+2p, ...),
// результат собирается на rank 0 через MPI_Gatherv.
// Сборка: mpicc -O3 mpi.c -o mpi -lm
//
// Почему циклически: тяжёлые строки (через середину множества) идут подряд.
// При блочной раздаче они достаются нескольким средним процессам, остальные ждут.
// Для сравнения можно запустить блочную раздачу: MANDEL_DIST=block
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mandel.h"

// Номер глобальной строки для k-й локальной строки процесса r
static inline int global_row(int k, int r, int p, int n, int block) {
    if (!block) return r + k * p;
    int base = n / p, rem = n % p;
    int start = r * base + (r < rem ? r : rem);
    return start + k;
}

// Сколько строк у процесса r. Одинаково для обеих раздач:
// N/p строк каждому и ещё по одной первым N%p процессам.
static inline int rows_of(int r, int p, int n) {
    return n / p + (r < n % p ? 1 : 0);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int n = mandel_size();
    const char *dist = getenv("MANDEL_DIST");
    int block = dist && strcmp(dist, "block") == 0;

    int my_rows = rows_of(rank, p, n);
    int *local = malloc((size_t)(my_rows > 0 ? my_rows : 1) * n * sizeof(int));

    // counts/displs для Gatherv (в элементах int), нужны только root
    int *counts = NULL, *displs = NULL, *recv = NULL, *img = NULL;
    if (rank == 0) {
        counts = malloc(p * sizeof(int));
        displs = malloc(p * sizeof(int));
        for (int i = 0; i < p; i++) {
            counts[i] = rows_of(i, p, n) * n;
            displs[i] = (i == 0) ? 0 : displs[i - 1] + counts[i - 1];
        }
        recv = malloc((size_t)n * n * sizeof(int));
        img  = malloc((size_t)n * n * sizeof(int));
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    // 1) вычисление своих строк
    for (int k = 0; k < my_rows; k++) {
        int y = global_row(k, rank, p, n, block);
        int *row = local + (long)k * n;
        for (int x = 0; x < n; x++) row[x] = mandel_pixel(x, y, n);
    }
    double t_comp = MPI_Wtime() - t0;

    // 2) сбор на rank 0: процесс i кладёт свой кусок по смещению displs[i]
    double t1 = MPI_Wtime();
    MPI_Gatherv(local, my_rows * n, MPI_INT,
                recv, counts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // 3) на root переставляем строки на их настоящие места
    if (rank == 0) {
        for (int i = 0; i < p; i++) {
            int ri = rows_of(i, p, n);
            for (int k = 0; k < ri; k++) {
                int y = global_row(k, i, p, n, block);
                memcpy(img + (long)y * n, recv + displs[i] + (long)k * n, n * sizeof(int));
            }
        }
    }
    double t_end = MPI_Wtime();
    double t_gath = t_end - t1;
    double t_total = t_end - t0;

    // Время запуска = время самого медленного процесса
    double T, cmin, cmax, gmax;
    MPI_Reduce(&t_total, &T,    1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_comp,  &cmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_comp,  &cmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_gath,  &gmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        uint64_t sum = mandel_checksum(img, n);
        printf("%s;%d;%.6f;%s\n", block ? "mpi_block" : "mpi", p, T, mandel_check(sum, 0));
        // Строка для анализа: дисбаланс счёта и цена сбора
        printf("#detail;%s;%d;comp_min=%.6f;comp_max=%.6f;gather=%.6f\n",
               block ? "mpi_block" : "mpi", p, cmin, cmax, gmax);
        free(counts); free(displs); free(recv); free(img);
    }
    free(local);
    MPI_Finalize();
    return 0;
}
