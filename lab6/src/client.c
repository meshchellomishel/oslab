#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <jansson.h>

#include "parallel_utils.h"

#define DEFAULT_PATH_TO_CONF "./config.conf"

struct Server {
  char ip[255];
  int port;
};

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

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

struct addr {
  char ip[16];
  uint64_t port;
};

int str2addr(char *string, struct addr *addr) {
  char port[6] = {};
  char *token = NULL;

  token = strtok(string, ":");
  if (!token)
    return -1;

  strcpy(&addr->ip, token);

  token = strtok(NULL, ":");
  if (!token)
    return -1;

  strcpy(&port, token);
  
  addr->port = atoi(&port);
  if (addr->port <= 0)
    return -1;

  return 0;
}

int main(int argc, char **argv) {
  int ret;
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  struct DistributeArgs *args = DistributeArgsAlloc(servers_num);
  if (!args) {
    printf("[ERROR]: Failed to allocate args\n");
    goto on_error;
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num = 1;
  struct addr *to = malloc(sizeof(struct addr) * servers_num);
  if (!to) {
    printf("[ERROR]: Failedd to allocate server addrs\n");
    goto on_error;
  }

  int fd;
  fd = open(DEFAULT_PATH_TO_CONF, O_RDONLY);
  if (fd < 0) {
    printf("[ERROR]: Failed to open config file '%s'\n", DEFAULT_PATH_TO_CONF);
    return -1;
  }

  // TODO: work continiously, rewrite to make parallel
  for (int i = 0; i < servers_num; i++) {
    char buf[21] = {}, elem;
    while (elem != '\n') {
      ret = read(fd, &elem, 1);
      if (ret <= 0) {
        printf("[ERROR]: failed to read from file\n");
        goto on_error;
      }
      if (elem != '\n')
        strcat(&buf, &elem);
    }
    ret = str2addr(&buf, &to[i]);
    if (ret < 0) {
      printf("[ERROR]: Failed to get addr\n");
      goto on_error;
    }
    struct hostent *hostname = gethostbyname(to[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      goto on_error;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      goto on_error;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      goto on_error;
    }

    struct FactorialArgs *commonArgs = args->CommonArgs;
    struct FactorialArgs *localArgs = args->LocalArgs;

    commonArgs->begin = 1;
    commonArgs->end = k;
    commonArgs->mod = mod;

    for (int i = 0; i < servers_num;i++) {
      Calculate_thread_args(commonArgs, localArgs);
      localArgs->mod = commonArgs->mod;

      char task[sizeof(uint64_t) * 3];
      memcpy(task, &localArgs->begin, sizeof(uint64_t));
      memcpy(task + sizeof(uint64_t), &localArgs->end, sizeof(uint64_t));
      memcpy(task + 2 * sizeof(uint64_t), &localArgs->mod, sizeof(uint64_t));

      printf("[DEBUG]: Server'%d': '%d' -> '%d'\n", i, localArgs->begin, localArgs->end);

      if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "[ERROR]: Server%d: Send failed\n", i);
        goto on_error;
      }
    }

    for (int i = 0; i < servers_num; i++) {
      char response[sizeof(uint64_t)];
      if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "[ERROR]: Server%d: Recieve failed\n", i);
        goto on_error;
      }

      uint64_t answer = 0;
      memcpy(&answer, response, sizeof(uint64_t));
      printf("[INFO]: Server%d: answer: %llu\n", i, answer);
    }

    close(sck);
  }
  free(to);

  return 0;

on_error:
  DistributeArgsFree(args);
  exit(-1);
}
