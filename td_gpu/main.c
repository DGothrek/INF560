#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include "add_vectors.h"


  
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
