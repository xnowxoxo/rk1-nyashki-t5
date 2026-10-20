#!/bin/bash
#SBATCH --job-name=rk1-t5-omp
#SBATCH --partition=standard
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --time=00:30:00
#SBATCH --output=logs/omp_%j.out

# Запуск из корня репозитория:  sbatch slurm/omp.sbatch

cd "$SLURM_SUBMIT_DIR"
echo "# node: $(hostname)"
echo "version;p;time_s;CHECK"

# Эталон нужен для CHECK; если его ещё нет — один запуск seq
[ -f ref_4096.txt ] || ./bin/seq

export OMP_PLACES=cores
export OMP_PROC_BIND=close

for sched in static dynamic,16 guided; do
    export OMP_SCHEDULE=$sched
    for t in 1 2 4 8 16 32; do
        export OMP_NUM_THREADS=$t
        for r in 1 2 3; do
            ./bin/omp
        done
    done
done
