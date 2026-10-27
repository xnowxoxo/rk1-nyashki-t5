#!/bin/bash
#SBATCH --job-name=t5_omp
#SBATCH --partition=standard
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --exclusive
#SBATCH --mem=16G
#SBATCH --nodelist=pbcn02   # pbcn01/pbcn03 have no C headers, gcc fails there
#SBATCH --time=00:40:00
#SBATCH --output=logs/omp_%j.out
# Track 5, Mandelbrot. Submit from the track folder: sbatch slurm/omp.sbatch

gcc -O3 src/seq.c -o seq -lm || exit 1
gcc -O3 -fopenmp src/omp.c -o omp -lm || exit 1
echo "# node: $(hostname)"
lscpu | grep -E '^(Model name|Socket|Core|Thread|NUMA node[0-9])' | sed 's/^/# /'
echo "# $(gcc --version | head -1)"

export OMP_PLACES=cores OMP_PROC_BIND=close
echo "version;p;time_s;CHECK"

# T1: sequential reference; its hash is what every parallel run must reproduce
for r in 1 2 3; do
  out=$(./seq)
  echo "$out"
done
REF=$(echo "$out" | sed -n 's/.*hash=\([0-9]*\).*/\1/p')

for s in static static,1 dynamic,1 guided; do
  for p in 1 2 4 8 16 32; do
    for r in 1 2 3; do
      OMP_SCHEDULE=$s OMP_NUM_THREADS=$p ./omp "$REF"
    done
  done
done
