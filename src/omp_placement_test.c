// Side test: the same count() loop, one thread, with the counter array at different addresses
// (global, stack, stack aligned 64 / 4096, global 2D row, heap aligned 4096). Shows that the histogram
// time at p = 1 depends on where the counters lie, not on the code.  Run: sbatch slurm/placement_test.sbatch
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hist.h"
static long long g[K];
static long long rows[64][K] __attribute__((aligned(4096)));
int main(){
  int *a=malloc(N*sizeof*a);
  for(long i=0;i<N;i++)a[i]=value(i);
  printf("a=%p g=%p rows=%p\n",(void*)a,(void*)g,(void*)rows);
  for(int r=0;r<3;r++){
    double t0=omp_get_wtime(); count(a,0,N,g);
    double t1=omp_get_wtime(); { long long h[K]={0}; count(a,0,N,h); t0=t1-t0; t1=omp_get_wtime()-t1; printf("stack=%p ",(void*)h);}
    double t2=omp_get_wtime(); { long long h[K] __attribute__((aligned(64)))={0}; count(a,0,N,h); t2=omp_get_wtime()-t2;}
    double t3=omp_get_wtime(); { long long h[K] __attribute__((aligned(4096)))={0}; count(a,0,N,h); t3=omp_get_wtime()-t3;}
    double t4=omp_get_wtime(); count(a,0,N,rows[1]); t4=omp_get_wtime()-t4;
    long long *hm=aligned_alloc(4096,4096); for(int b=0;b<K;b++)hm[b]=0;
    double t5=omp_get_wtime(); count(a,0,N,hm); t5=omp_get_wtime()-t5;
    printf("global=%.4f stack=%.4f stack64=%.4f stack4k=%.4f rows=%.4f heap4k=%.4f\n",t0,t1,t2,t3,t4,t5);
  }
}
