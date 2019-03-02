#include <string.h>
#include <stdio.h>
#include <stdlib.h>


__global__ void vectAdd(char *a, char *b, char *c, char *res, int len)
{
  int i;
  i = blockIdx.x * blockDim.x + threadIdx.x;

  res[i] = a[i] + b[i] + c[i];
}

/* Function computing the final string to print */
void compute_string(char *res, char *a, char *b, char *c, int length)
{
  char *d_a, *d_b, *d_c, *d_res;

  cudaMalloc(&d_a, length * sizeof(char));
  cudaMalloc(&d_b, length * sizeof(char));
  cudaMalloc(&d_c, length * sizeof(char));
  cudaMalloc(&d_res, length * sizeof(char));

  cudaMemcpy(d_a, a, length * sizeof(char), cudaMemcpyHostToDevice);
  cudaMemcpy(d_b, b, length * sizeof(char), cudaMemcpyHostToDevice);
  cudaMemcpy(d_c, c, length * sizeof(char), cudaMemcpyHostToDevice);

  dim3 dimBlock(30);
  dim3 dimGrid(1);
  vectAdd<<<dimGrid, dimBlock>>>(d_a, d_b, d_c, d_res, length);

  cudaMemcpy(res, d_res, length * sizeof(char), cudaMemcpyDeviceToHost);

  cudaFree(d_a);
  cudaFree(d_b);
  cudaFree(d_c);
  cudaFree(d_res);
}


int main()
{

  char *res;

  char a[30] = {40, 70, 70, 70, 80, 0, 50, 80, 80, 70, 70, 0, 40, 80, 79,
                70, 0, 40, 50, 50, 0, 70, 80, 0, 30, 50, 30, 30, 0, 0};
  char b[30] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
                10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 0, 0};
  char c[30] = {22, 21, 28, 28, 21, 22, 27, 21, 24, 28, 20, 22, 20, 24, 22,
                29, 22, 21, 20, 25, 22, 25, 20, 22, 27, 25, 28, 25, 0, 0};

  res = (char *)malloc(30 * sizeof(char));

  /* This function call should be programmed in CUDA */
  /* -> need to allocate and transfer data to/from the device */
  compute_string(res, a, b, c, 30);

  printf("%s\n", res);

  return 0;
}
