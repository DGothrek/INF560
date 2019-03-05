#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

__global__ void vectAdd(int *a, int *b, int *c, int len)
{
  int i;
  i = blockIdx.x * blockDim.x + threadIdx.x;

  if (i < len)
  {
    c[i] = a[i] + b[i];
  }
}

/* Function computing the final string to print */
void vector_add(int *c, int *a, int *b, int length)
{
  int *d_a, *d_b, *d_c;
  struct timeval t1, t2;

  cudaMalloc(&d_a, length * sizeof(int));
  cudaMalloc(&d_b, length * sizeof(int));
  cudaMalloc(&d_c, length * sizeof(int));

  gettimeofday(&t1, NULL);

  cudaMemcpy(d_a, a, length * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_b, b, length * sizeof(int), cudaMemcpyHostToDevice);

  gettimeofday(&t2, NULL);
  printf("Transfer done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
  gettimeofday(&t1, NULL);

  // a (1D/2D/3D) grid containing blocks, each one containing threads.
  dim3 dimGrid(length / 1024 + 1);
  dim3 dimBlock(1024);
  vectAdd<<<dimGrid, dimBlock>>>(d_a, d_b, d_c, length);

  gettimeofday(&t2, NULL);
  printf("Processing done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
  gettimeofday(&t1, NULL);

  cudaMemcpy(c, d_c, length * sizeof(int), cudaMemcpyDeviceToHost);

  gettimeofday(&t2, NULL);
  printf("Tranfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
  gettimeofday(&t1, NULL);

  cudaFree(d_a);
  cudaFree(d_b);
  cudaFree(d_c);
}

int main(int argc, char **argv)
{
  int N, S;

  if (argc < 2)
  {
    printf("Usage: %s N S\n", argv[0]);
    printf("\tS: seed for pseudo-random generator\n");
    printf("\tN: size of the array\n");
    exit(1);
  }

  N = atoi(argv[1]);
  S = atoi(argv[2]);
  srand48(S);

  int *A, *B, *C;
  int i;
  A = (int *)malloc(N * sizeof(int));
  B = (int *)malloc(N * sizeof(int));
  C = (int *)malloc(N * sizeof(int));

  /* Initialize the array */
  for (i = 0; i < N; i++)
  {
    A[i] = lrand48();
    B[i] = lrand48();
  }

  vector_add(C, A, B, N);

  /* Checking the result */
  printf("Checking the result...\n");
  for (i = 0; i < N; i++)
  {
    if (C[i] != A[i] + B[i])
    {
      printf("Wrong res for i=%d\n", i);
      return 0;
    }
  }

  printf("Res OK!\n");

  return 0;
}
