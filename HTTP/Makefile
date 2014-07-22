COMPILER = gcc
CFLAGS = -Wall -o
all: http
http: httpproxyserver.o httpclient.o
httpproxyserver.o:
	$(COMPILER) httpproxyserver.c $(CFLAGS) httpproxyserver
httpclient.o:
	$(COMPILER) httpclient.c $(CFLAGS) httpclient
clean:
	rm -rf *.o http http.o
