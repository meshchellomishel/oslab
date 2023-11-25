tcpserver: tcpserver.c Makefile
	gcc tcpserver.c -o tcpserver

udpserver: udpserver.c Makefile
	gcc udpserver.c -o udpserver

tcpclient: tcpclient.c Makefile
	gcc tcpclient.c -o tcpclient

udpclient: udpclient.c Makefile
	gcc udpclient.c -o udpclient

all: tcpserver udpserver tcpclient udpclient

clean:
	rm udpclient tcpclient udpserver tcpserver