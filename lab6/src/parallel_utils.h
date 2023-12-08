#include <netinet/in.h>

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

struct DistributeArgs {
    struct FactorialArgs *LocalArgs;
    struct FactorialArgs *CommonArgs;
    uint64_t tnum;
};

void DistributeArgsFree(struct DistributeArgs *args);
struct DistributeArgs *DistributeArgsAlloc(int tnum);
void Calculate_thread_args(struct FactorialArgs *common, struct FactorialArgs *local, int pid, int pnum);
int Calculate_all_args(struct FactorialArgs *common, struct FactorialArgs *local, int pnum);