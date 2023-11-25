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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "parallel_utils.h"

#define DEFAULT_PATH_TO_CONF "./config.conf"
#define DEFAULT_THREADS_NUM 2
#define DEFAULT_MOD 100

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
  int fd;
  struct FactorialArgs *local;
  char ip[16];
  uint64_t port;
};

int str2addr(char *string, struct addr *addr) {
  char port[6] = {};
  char *token = NULL;

  token = strtok(string, ":");
  if (!token)
    return -1;

  strcpy((char *)&addr->ip, token);

  token = strtok(NULL, ":");
  if (!token)
    return -1;

  strcpy((char *)&port, token);
  
  addr->port = atoi((const char *)&port);
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

  if (k == -1) {
    printf("[INFO]: Using default k\n");
    k = DEFAULT_THREADS_NUM;
  }
  if (mod == -1) {
    printf("[INFO]: Using default mod\n");
    mod = DEFAULT_MOD;
  }
  if (!strlen(servers)) {
    printf("[INFO]: Using default server path\n");
    strcpy((char *)&servers, DEFAULT_PATH_TO_CONF);
  }

  // TODO: for one server here, rewrite with servers from file
  unsigned int servers_num = 2;
  struct addr *to = malloc(sizeof(struct addr) * servers_num);
  if (!to) {
    printf("[ERROR]: Failedd to allocate server addrs\n");
    goto on_error;
  }

  struct DistributeArgs *args = DistributeArgsAlloc(servers_num);
  if (!args) {
    printf("[ERROR]: Failed to allocate args\n");
    goto on_error;
  }

  struct FactorialArgs *commonArgs = args->CommonArgs;
  struct FactorialArgs *localArgs = args->LocalArgs;

  commonArgs->begin = 1;
  commonArgs->end = k;
  commonArgs->mod = mod;

  int fd;
  fd = open(DEFAULT_PATH_TO_CONF, O_RDONLY);
  if (fd < 0) {
    printf("[ERROR]: Failed to open config file '%s'\n", DEFAULT_PATH_TO_CONF);
    return -1;
  }

  // TODO: work continiously, rewrite to make parallel
  for (int i = 0; i < servers_num; i++) {
    unsigned char buf[21] = {};
    unsigned char elem = '\0';
    int j = 0;

    while (elem != '\n') {
      ret = read(fd, &elem, sizeof(elem));
      if (ret < 0) {
        printf("[ERROR]: failed to read from config file\n");
        goto on_error;
      }

      buf[j] = elem;
      j++;
    }
    printf("\n[INFO]: Server%d %s\n", i, &buf);
    ret = str2addr((char *)&buf, &to[i]);
    if (ret < 0) {
      printf("[ERROR]: Failed to get addr\n");
      goto on_error;
    }


    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      printf("[ERROR]: Socket creation failed!\n");
      goto on_error;
    }
    to[i].fd = sck;
    
    printf("[DEBUG]: Ip: %s, Port: %d\n", to[i].ip, to[i].port);

    struct hostent *hostname = gethostbyname("localhost");
    if (hostname == NULL) {
      fprintf(stderr, "[ERROR]: gethostbyname failed with %s\n", to[i].ip);
      goto on_error;
    }
 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);
    printf("[DEBUG]: Connecting to Server%d\n", i);

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      printf("[ERROR]: Connection failed to Server%d\n", i);
      goto on_error;
    }
  }

  printf("[DEBUG]: Distributing args\n");
  for (int i = 0;i < servers_num; i++) {
      Calculate_thread_args(commonArgs, localArgs, i, servers_num);
      localArgs->mod = commonArgs->mod;

      printf("[DEBUG]: Server'%d': '%d' -> '%d'\n", i, localArgs->begin, localArgs->end);
      to[i].local = localArgs;
  }

  for (int i = 0; i < servers_num;i++) {
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &localArgs->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &localArgs->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &localArgs->mod, sizeof(uint64_t));

    if (send(to[i].fd, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "[ERROR]: Server%d: Send failed\n", i);
      goto on_error;
    }
  }

  for (int i = 0; i < servers_num; i++) {
    char response[sizeof(uint64_t)];
    if (recv(to[i].fd, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "[ERROR]: Server%d: Recieve failed\n", i);
      goto on_error;
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    printf("[INFO]: Server%d: answer: %llu\n", i, answer);
    close(to[i].fd);
  }

  free(to);

  return 0;

on_error:
  close(fd);
  DistributeArgsFree(args);
  exit(-1);
}
