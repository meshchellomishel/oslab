#include <netinet/in.h>

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    int pid;
};

struct DistributeArgs {
    struct FactorialArgs *LocalArgs;
    struct FactorialArgs *CommonArgs;
    uint64_t tnum;
};

void Calculate_thread_args(struct DistributeArgs *args);