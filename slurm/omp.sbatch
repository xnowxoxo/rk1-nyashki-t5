#!/bin/bash
#SBATCH --job-name=t11_omp
#SBATCH --partition=standard
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --exclusive
#SBATCH --mem=16G
#SBATCH --nodelist=pbcn02   # pbcn01/pbcn03 have no C headers, gcc fails there
#SBATCH --time=00:40:00
#SBATCH --output=logs/omp_%j.out
# Track 11, histogram + counting sort. Submit from the track folder: sbatch slurm/omp.sbatch

gcc -O3 src/seq.c -o seq || exit 1
gcc -O3 -fopenmp src/omp.c -o omp || exit 1
echo "# node: $(hostname)"
lscpu | grep -E '^(Model name|Socket|Core|Thread|NUMA node[0-9])' | sed 's/^/# /'
echo "# $(gcc --version | head -1)"

# spread: memory-bound kernel, so use both sockets' memory bandwidth from p = 2 on
export OMP_PLACES=cores OMP_PROC_BIND=spread
echo "version;p;time_s;CHECK"

for r in 1 2 3; do ./seq; done

for v in atomic local; do
  for p in 1 2 4 8 16 32; do
    for r in 1 2 3; do
      OMP_NUM_THREADS=$p ./omp $v
    done
  done
done
