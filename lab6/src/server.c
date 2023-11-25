#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "parallel_utils.h"

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }

  return result % mod;
}

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  
  for (int i = args->begin; i < args->end; i++)
    ans *= i;

  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        if (port <= 0)
          printf("[ERROR]: Invalid arg: Port number must be positive numben\n");
        break;
      case 1:
        tnum = atoi(optarg);
        if (tnum < 0)
          printf("[ERROR]: Invalid arg: Thread number must be positive numben\n");
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    goto on_error;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    goto on_error;
  }

  struct DistributeArgs *args = DistributeArgsAlloc(tnum);

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!");
    goto on_error;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    goto on_error;
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read = recv(client_fd, from_client, buffer_size, 0);

      if (!read)
        break;
      if (read < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }
      if (read < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      pthread_t threads[tnum];

      struct FactorialArgs *commonArgs = args->CommonArgs;
      struct FactorialArgs *localArgs = args->LocalArgs;

      memcpy(&commonArgs->begin, from_client, sizeof(uint64_t));
      memcpy(&commonArgs->end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&commonArgs->mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %llu %llu %llu\n", commonArgs->begin, commonArgs->end, commonArgs->mod);
      if (commonArgs->begin == 0 || commonArgs->end == 0 || commonArgs->mod == 0)
        fprintf(stdout, "[ERROR]: Invalid input: begin = 0 or end = 0 or mod = 0\n");

      for (uint32_t i = 0; i < tnum; i++) {
        Calculate_thread_args(commonArgs, &localArgs[i], i, tnum);
        printf("[DEBUG]: th%d: from '%d' -> '%d'\n", i, localArgs[i].begin, localArgs[i].end);

        if (pthread_create(&threads[i], NULL, ThreadFactorial,
                           (void *)&localArgs[i])) {
          printf("Error: pthread_create failed!\n");
          goto on_error;
        }
      }

      uint64_t total = 1;
      for (uint32_t i = 0; i < tnum; i++) {
        uint64_t result = 0;
        pthread_join(threads[i], (void **)&result);
        total *= result;
      }

      printf("Total: %llu\n", total);

      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }
  }
  
  return 0;

  on_error:
    DistributeArgsFree(args);
    exit(-1);
}
