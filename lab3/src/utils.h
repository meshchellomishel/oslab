#ifndef UTILS_H
#define UTILS_H

#define PRINT_MAX_BOUND      3
#define PRINT_MIN_SKIP_LINES 2
static inline int start_skip(int index, int len)
{
    return (index == PRINT_MAX_BOUND - 1 && len - index - PRINT_MAX_BOUND + 1 >= PRINT_MIN_SKIP_LINES);
}

struct MinMax {
  int min;
  int max;
};

void GenerateArray(int *array, unsigned int array_size, unsigned int seed);
void PrintArray(int *array, int array_size);

#endif
