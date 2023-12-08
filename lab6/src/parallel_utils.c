#include <stdio.h>
#include <stdlib.h>

#include "parallel_utils.h"

struct DistributeArgs *DistributeArgsAlloc(int tnum) {
    struct DistributeArgs *args = NULL;

    args = malloc(sizeof(struct DistributeArgs *));
    if (!args) {
        printf("[ERROR]: Failed to allocate args\n");
        goto on_error;
    }

    args->LocalArgs = NULL;

    args->LocalArgs = malloc(sizeof(struct FactorialArgs) * tnum);
    if (!args->LocalArgs) {
        printf("[ERROR]: Failed to allocate local args\n");
        goto on_error;
    }

    args->CommonArgs = NULL;

    args->CommonArgs = malloc(sizeof(struct FactorialArgs *));
    if (!args->CommonArgs) {
        printf("[ERROR]: Failed to allocate common args\n");
        goto on_error;
    }

    return args;

on_error:
    return NULL;
}

void DistributeArgsFree(struct DistributeArgs *args) {
    if (args) {
        if (args->LocalArgs)
            free(args->LocalArgs);
        
        if (args->CommonArgs)
            free(args->CommonArgs);

        free(args);
    }
}

int Calculate_all_args(struct FactorialArgs *common, struct FactorialArgs *local, int pnum) {
  int buf, workers = 0;
  int chunk = (common->end - common->begin + 1) / pnum;
  if (!chunk)
    chunk = 1;

  local[0].begin = common->begin;
  if (local[0].begin == common->end) {
      local[0].end = local[0].begin;
      return workers + 1;
    }

  local[0].end = common->begin + chunk;
  workers++;

  for (int i = 1;i < pnum - 1;i++) {
    if (local[i-1].end == common->end)
      return workers;

    local[i].begin = local[i-1].end + 1;
    if (local[i].begin == common->end) {
      local[i].end = common->end;
      return workers + 1;
    }

    local[i].end = local[i].begin + chunk;
    workers++;
  }

  if (local[pnum-2].end == common->end)
      return workers;

  local[pnum-1].begin = local[pnum-2].end + 1;
  local[pnum-1].end = common->end;
  return workers + 1;
}