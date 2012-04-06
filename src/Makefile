CC = gcc
DEBUG =	-g -ggdb -DDEBUG
CFLAGS =  -c -W -Wall -Werror -fPIC  $(DEBUG)

LIB_OBJS = \
	./http_parser/http_parser.o \
	./picoev/picoev_kqueue.o

freepush-server: server.c $(LIB_OBJS)
	$(CC) -o freepush-server server.c $(LIB_OBJS)

http_parser/http_parser.o:
	$(CC) -o ./http_parser/http_parser.o http_parser/http_parser.c

picoev/picoev_kqueue.o:
	$(CC) -o ./picoev/picoev_kqueue.o -I ./picoev picoev/picoev_kqueue.c

clean:
	-rm -f $(LIB_OBJS)


