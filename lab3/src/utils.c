#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
void GenerateArray(int *array, unsigned int array_size, unsigned int seed) {
  srand(seed);
  for (int i = 0; i < array_size; i++) {
    array[i] = rand() % 20;
  }
}

void PrintArray(int *arr, int array_size)
{
  int N = array_size;

  printf("[ ");
  for (int i = 0; i < N - 1; i++) {
      printf("%d, ", arr[i]);
      if (start_skip(i, N)) {
          printf("\t...\t");
          i = N - PRINT_MAX_BOUND;
      }
  }
  printf("%d ]\n", arr[N - 1]);
}