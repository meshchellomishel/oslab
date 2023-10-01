#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

#define MAX_PNUM 10

struct args {
  int seed;
  int array_size;
  int pnum;
  bool with_files;
};

struct process {
  int id;
  int fd[2];
  int pid;
  int read_fd;
  int write_fd;
  char *filepath;
};

struct ctx {
  struct args args;
  int *array;
  struct MinMax info;
  struct process *processes;
};

void generate_filepath_by_id(char *str, int id)
{
  sprintf(str, "./lab-fd-%d.txt", id);
}

void process_init(struct process *process)
{
  process->filepath = NULL;
}

int process_alloc(struct process *process, bool with_files)
{
  int fd;

  if (with_files) {
    process->filepath = malloc(sizeof(char *) * 256);
    if (!process->filepath) {
      printf("Can`t malloc a filepath\n");
      return -1;
    }
    generate_filepath_by_id(process->filepath, process->id);
    printf("INFO: Child file is %s\n", process->filepath);

    process->read_fd = 0;
    process->write_fd = 0;
  } else {
    pipe(process->fd);
    process->read_fd = process->fd[0];
    process->write_fd = process->fd[1];
  }

  return 0;
}

void process_destroy(struct process *process)
{
    close(process->read_fd);
    close(process->write_fd);
}

void ctx_init(struct ctx *ctx)
{
  ctx->args.pnum = 0;
  ctx->args.seed = 0;
  ctx->args.array_size = 0;
  ctx->args.with_files = false;
  ctx->array = NULL;
  ctx->processes = NULL;
}

int ctx_alloc(struct ctx *ctx)
{
  ctx->array = malloc(sizeof(int) * ctx->args.array_size);
  if (!ctx->array)
    return -1;

  ctx->processes = malloc(sizeof(struct process) * ctx->args.pnum);
  if (!ctx->processes)
    return -1;

  ctx->info.min = INT_MAX;
  ctx->info.max = INT_MIN;
  return 0;
}

int ctx_processes_alloc(struct ctx *ctx)
{
  int ret;

  for (int i = 0;i < ctx->args.pnum;i++) {
    struct process *process = &ctx->processes[i];
    process_init(process);
    process->id = i;

    ret = process_alloc(process, ctx->args.with_files);
    if (ret < 0) {
      printf("ERROR: Process allocate failed\n");
      return -1;
    }
  }
}

int ctx_destroy(struct ctx *ctx) 
{
  int ret;

  if (ctx->processes) {
    for (int i = 0; i < ctx->args.pnum; i++) {
      struct process *process = &ctx->processes[i];

      if (process) {
        if (ctx->args.with_files) {
          process_destroy(process);
          if (process->filepath) {
            ret = remove(process->filepath);
            if (ret < 0) {
              printf("ERROR: Can`t delete file '%s'", process->filepath);
              return -1;
            }
            free(process->filepath);
          }
        }
      }
    }
  }
  if (ctx->array) {
    free(ctx->array);
  }
  if (ctx->processes)
    free(ctx->processes);
}

void on_kill_timer(struct ctx *ctx)
{
  int ret;

  for (int i = 0;i < ctx->args.pnum;i++) {
    ret = kill(ctx->processes[i].pid, 9);
    if (ret < 0)
      printf("ERROR: Process with pid %d sending SIGKILL failed\n");
  }

  ctx_destroy(ctx);
}

void on_error(struct ctx *ctx, int error)
{
  ctx_destroy(ctx);
  exit(error);
}

int parse_args(struct ctx *ctx, int argc, char **argv)
{
    while (true) {
      int current_optind = optind ? optind : 1;

      static struct option options[] = {{"seed", required_argument, 0, 0},
                                        {"array_size", required_argument, 0, 0},
                                        {"pnum", required_argument, 0, 0},
                                        {"by_files", no_argument, 0, 'f'},
                                        {0, 0, 0, 0}};

      int option_index = 0;
      int c = getopt_long(argc, argv, "f", options, &option_index);

      if (c == -1) break;

      switch (c) {
        case 0:
          switch (option_index) {
            case 0:
              ctx->args.seed = atoi(optarg);
              if (ctx->args.seed <= 0) {
                printf("seed is a positive number\n");
                return -1;
              }
              break;
            case 1:
              ctx->args.array_size = atoi(optarg);
              if (ctx->args.array_size <= 0) {
                printf("array_size is a positive number\n");
                return -1;
              }
              break;
            case 2:
              ctx->args.pnum = atoi(optarg);
              if (ctx->args.pnum <= 0 || ctx->args.pnum >= MAX_PNUM) {
                printf("pnum is a positive number\n");
                return -1;
              }
              break;
            case 3:
              ctx->args.with_files = true;
              break;

            defalut:
              printf("Index %d is out of options\n", option_index);
          }
          break;
        case 'f':
          ctx->args.with_files = true;
          break;

        case '?':
          break;

        default:
          printf("getopt returned character code 0%o?\n", c);
      }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return -1;
  }

  if (!ctx->args.seed || !ctx->args.array_size || !ctx->args.pnum) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return -1;
  }

  return 0;
}

int do_fork(int *array, struct process *process, int begin, int end)
{
  pid_t child_pid = fork();
  if (child_pid >= 0) {
    if (child_pid == 0) {
      // child process
      int ret;
      struct MinMax min_max;

      min_max = GetMinMax(array, begin, end);

      printf("INFO: Child result is {%d, %d}\n", min_max.min, min_max.max);

      if (!process->write_fd) {
        process->write_fd = open(process->filepath, O_CREAT | O_RDWR, S_IRWXU);
        if (process->write_fd < 0) {
          printf("ERROR: Can`t open a file\n");
          exit(-1);
        }
      }

      ret = write(process->write_fd, &min_max, sizeof(struct MinMax));
      if (ret < 0) {
        printf("ERROR: Can`t write to fd\n");
        exit(-1);
      }

      close(process->write_fd);
      exit(0);
      
    } else {
      printf("INFO: Child PID is %d\n", child_pid);
      process->pid = child_pid;
    }

  } else {
    printf("Fork failed!\n");
    return -1;
  }

  return 0;
}

int read_from_processes(struct ctx *ctx)
{
  int ret, read_fd;
  struct MinMax min_max;
  printf("INFO: Reading from processes...\n");

  for (int i = 0; i < ctx->args.pnum; i++) {
    read_fd = ctx->processes[i].read_fd;

    if (ctx->args.with_files) {
      read_fd = open(ctx->processes[i].filepath, O_RDWR);
      if (read_fd < 0) {
        printf("ERROR: Can`t open a file '%s'\n", ctx->processes[i].filepath);
        printf("\t\terror message: %s\n", strerror(errno));
        return -1;
      }
    }
    ret = read(read_fd, &min_max, sizeof(struct MinMax));
    if (ret <= 0)
      return -1;
    
    printf("INFO: child recv {%d, %d}\n", min_max.min, min_max.max);

    if (min_max.min < ctx->info.min) ctx->info.min = min_max.min;
    if (min_max.max > ctx->info.max) ctx->info.max = min_max.max;

    close(read_fd);
  }
}

int do_parallel(int *array, struct process *process, int begin, int end, bool with_files)
{
  int ret;

  printf("DEBUG: begin = %d, end = %d\n", begin, end);

  ret = do_fork(array, process, begin, end);
  if (ret < 0) {
    printf("ERROR: do_fork failed\n");
    return -1;
  }
  printf("INFO: fork done\n");

  return 0;
}

int distribute_parallel(struct ctx *ctx, int chunk)
{
  int begin, end, ret;
  int active_child_processes = 0;
  bool with_files = ctx->args.with_files;
  
  if (chunk == 0)
    chunk = ctx->args.array_size;

  begin = 0;
  end = 0;
  for (int i = 0;i < ctx->args.pnum - 1;i++) {
    if (active_child_processes == i) {
      begin = end;
      end += chunk;
    }

    struct process *process = &ctx->processes[i];
    process->id = i;
    
    ret = do_parallel(ctx->array, process, begin, end, with_files);
    if (ret < 0) {
      printf("ERROR: do_parallel failed\n");
      continue;
    }
    active_child_processes += 1;
  }

  if (end - begin || ctx->args.pnum) {
    begin = end;
    end = ctx->args.array_size;

    struct process *process = &ctx->processes[active_child_processes];
    process->id = active_child_processes;

    ret = do_parallel(ctx->array, process, begin, end, with_files);
    if (ret < 0) {
      printf("ERROR: do_parallel failed\n");
      return -1;
    }

    active_child_processes += 1;
  }

  return active_child_processes;
}

int parallel(struct ctx *ctx, int chunk)
{
  int ret;
  int active_child_processes = 0;

  printf("INFO: Chunks...\n");

  active_child_processes = distribute_parallel(ctx, chunk);
  if (active_child_processes <= 0) {
    printf("ERROR: Failed to distribute processes\n");
    return -1;
  }

  printf("INFO: Waiting processes...\n");
  int status;
  bool can_continue = true;
  for (int i = 0;i < ctx->args.pnum;i++) {
    waitpid(ctx->processes[i].pid, &status, 0);
    if (WEXITSTATUS(status) != 0) {
      can_continue = false;
      printf("ERROR: Child with pid %d failed, exit status %d\n",
             ctx->processes[i].pid, WEXITSTATUS(status));
      printf("\t error message: %s\n", strerror(WEXITSTATUS(status)));
    }

    active_child_processes -= 1;
  }

  if (!can_continue) {
    printf("ERROR: Can`t can`t continue, child process failed\n");
    on_error(ctx, errno);
  }

  ret = read_from_processes(ctx);
  if (ret < 0) {
    printf("ERROR: Can`t read from processes");
    on_error(ctx, errno);
  }

  return 0;
}

int main(int argc, char **argv) {
  int ret, chunk;
  struct ctx ctx;

  ctx_init(&ctx);

  printf("INFO: Parsing args...\n");
  ret = parse_args(&ctx, argc, argv);
  if (ret < 0)
    on_error(&ctx, -ret);
  
  printf("With files = %d\n", false);
  
  printf("INFO: Context alloc...\n");
  ret = ctx_alloc(&ctx);
  if (ret < 0)
    on_error(&ctx, -ret);

  printf("INFO: Processes alloc...\n");
  ret = ctx_processes_alloc(&ctx);
  if (ret < 0)
    on_error(&ctx, -ret);

  GenerateArray(ctx.array, ctx.args.array_size, ctx.args.seed);
  PrintArray(ctx.array, ctx.args.array_size);

  chunk = ctx.args.array_size / (ctx.args.pnum);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  ret = parallel(&ctx, chunk);
  if (ret < 0) {
    on_error(&ctx, errno);
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  printf("Min: %d\n", ctx.info.min);
  printf("Max: %d\n", ctx.info.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  
  ctx_destroy(&ctx);
  return 0;
}
