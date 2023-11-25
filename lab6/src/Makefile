export LD_LIBRARY_PATH=./

client: client.c parallel_utils.o Makefile
	gcc parallel_utils.o client.c -o client

server: server.c parallel_utils.o Makefile
	gcc parallel_utils.o server.c -o server

parallel_utils: parallel_utils.h parallel_utils.c Makefile
	gcc -c parallel_utils.c -o parallel_utils.o

lib: parallel_utils.o parallel_utils
	gcc -shared parallel_utils.o -o parallel_utils.so

all: client server parallel_utils lib

clean:
	rm parallel_utils.so parallel_utils.o server client