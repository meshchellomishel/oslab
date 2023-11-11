parallel: parallel_min_max.c find_min_max.c find_min_max.h utils.h utils.c
	gcc -pthread  parallel_min_max.c find_min_max.c utils.c -o lab3

find_min_max: find_min_max.c find_min_max.h
	gcc find_min_max.c -o find_min_max

utils: utils.c utils.h
	gcc utils.c -o utils

all: parallel find_min_max utils