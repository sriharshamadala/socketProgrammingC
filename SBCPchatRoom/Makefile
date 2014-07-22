COMPILER = gcc
CFLAGS = -Wall -o
all: chat
chat: chatserver.o chatclient.o
chatserver.o:
	$(COMPILER) chatserver.c $(CFLAGS) chatserver
chatclient.o:
	$(COMPILER) chatclient.c $(CFLAGS) chatclient
clean:
	rm -rf *.o chat chat.o

