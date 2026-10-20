#!/bin/bash
#SBATCH --job-name=rk1-t5-mpi
#SBATCH --partition=standard
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=32
#SBATCH --time=00:30:00
#SBATCH --output=logs/mpi_%j.out

# Запуск из корня репозитория:  sbatch slurm/mpi.sbatch
# Бинарники собрать заранее на bilim-master (make), на узлах нет заголовков C.

module load intel/mpi/2021.14
# На PARAM BILIM Intel MPI работает через srun только так:
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi.so

cd "$SLURM_SUBMIT_DIR"
echo "# node: $(hostname); nodes: $SLURM_JOB_NODELIST"
echo "version;p;time_s;CHECK"

# T1: последовательная версия, 3 запуска (первый запуск создаёт эталон ref_4096.txt)
for r in 1 2 3; do
    srun --mpi=none -N 1 -n 1 ./bin/seq
done

# Основная серия: циклическая раздача строк, 1..64 процесса, по 3 запуска
for p in 1 2 4 8 16 32 64; do
    nodes=$(( p > 32 ? 2 : 1 ))
    for r in 1 2 3; do
        srun --mpi=none -N $nodes -n $p --cpu-bind=cores ./bin/mpi
    done
done

# Для анализа узкого места: блочная раздача тех же строк
for p in 16 64; do
    nodes=$(( p > 32 ? 2 : 1 ))
    for r in 1 2 3; do
        MANDEL_DIST=block srun --mpi=none -N $nodes -n $p --cpu-bind=cores ./bin/mpi
    done
done
