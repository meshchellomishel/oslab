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

    args->COmmonArgs = NULL;

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

void Calculate_thread_args(struct FactorialArgs *common, struct FactorialArgs *local) {
  int chunk = (common->end - common->begin + 1) / common->tnum;

  if (local->pid == 0)
    local->begin = 1;
  else
    local->begin = chunk * local->pid;

  if (local->pid + 1 == args->pnum)
    local->end = common->end;

  local->end = chunk * (local->pid + 1);
}