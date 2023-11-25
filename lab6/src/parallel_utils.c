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

    args->LocalArgs = malloc(sizeof(struct FactorialArgs *) * tnum);
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

void Calculate_thread_args(struct FactorialArgs *common, struct FactorialArgs *local, int pid, int pnum) {
  int chunk = (common->end - common->begin + 1) / pnum;

  if (pid == 0)
    local->begin = common->begin;
  else
    local->begin = chunk * pid;

  if (pid + 1 == pnum)
    local->end = common->end;

  local->end = chunk * (pid + 1);
}