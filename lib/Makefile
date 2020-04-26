CC=gcc
#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto
CFLAGS=-Wall -Wextra -pthread -O0 -ggdb3 -Wfatal-errors -Wno-unused-parameter -Wno-unused-function

BUILDDIR=build/

SOURCE=skat.c server.c connection.c atomic_queue.c card.c
OBJS=$(SOURCE:.c=.o)
EXTRA_HEADERS=

.PHONY: default clean png_gone distclean png

default: server client

server: $(OBJS) main_server.o 
	$(CC) $(CFLAGS) -o $@ $^

client: $(OBJS) main_client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o:%.c 
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) $(OBJS) main_server.o main_client.o dep_graph.png

distclean:
	$(RM) $(OBJS) main_server.o main_client.o dep_graph.png
	$(RM) server client

png_gone:
	$(RM) dep_graph.png

png:
	./dep_graph.sh
