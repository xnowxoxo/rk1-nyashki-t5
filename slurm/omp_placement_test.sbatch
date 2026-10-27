#!/bin/bash
#SBATCH --job-name=t11_place
#SBATCH --partition=standard
#SBATCH --nodelist=pbcn02
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --mem=8G
#SBATCH --time=00:05:00
#SBATCH --output=logs/placement_test_%j.out
# Submit from the track folder: sbatch slurm/placement_test.sbatch

gcc -O3 -fopenmp src/placement_test.c -o placement_test || exit 1
OMP_NUM_THREADS=1 OMP_PLACES=cores OMP_PROC_BIND=close ./placement_test
