#!/bin/bash
#SBATCH --job-name=rk1-t5-omp
#SBATCH --partition=standard
#SBATCH --exclude=pbgpu01         # только pbcn01–03: то же железо, что у seq и MPI
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48        # 2 сокета × 24 ядра; HT = 2, поэтому 96 логических CPU
#SBATCH --hint=nomultithread      # один CPU = одно физическое ядро, без HT-пар
#SBATCH --exclusive               # на узле нет чужих задач во время замеров
#SBATCH --time=00:30:00
#SBATCH --output=logs/omp_%j.out

# Запуск из корня репозитория:  sbatch slurm/omp.sbatch
# Бинарники собрать заранее на bilim-master (make), на узлах нет заголовков C.

cd "$SLURM_SUBMIT_DIR"
echo "# node: $(hostname)"
echo "version;p;time_s;CHECK"

# make кладёт в bin/ копию libgomp — на случай, если на узле её нет
export LD_LIBRARY_PATH="$SLURM_SUBMIT_DIR/bin:$LD_LIBRARY_PATH"
export OMP_PLACES=cores
export OMP_PROC_BIND=close

# Эталон нужен для CHECK; если его ещё нет — один запуск seq
[ -f ref_4096.txt ] || ./bin/seq

# Основная серия: 4 расписания, 1..48 потоков, по 3 запуска
#   static     — сплошные блоки по N/p строк
#   static,1   — строки по кругу (как циклическая раздача в MPI)
#   dynamic,1  — общая очередь по одной строке
#   guided,1   — порции убывают от N/p до 1
for sched in static static,1 dynamic,1 guided,1; do
    export OMP_SCHEDULE=$sched
    for t in 1 2 4 8 16 24 32 48; do
        export OMP_NUM_THREADS=$t
        for r in 1 2 3; do
            ./bin/omp
        done
    done
done

# Размер порции на всех 48 ядрах
export OMP_NUM_THREADS=48
for kind in static dynamic guided; do
    for c in 4 16 64; do
        export OMP_SCHEDULE=$kind,$c
        for r in 1 2 3; do
            ./bin/omp
        done
    done
done
