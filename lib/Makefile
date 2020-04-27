CC=gcc
#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto
CFLAGS=-Wall -Wextra -pthread -O0 -ggdb3 -Wfatal-errors -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas

BUILDDIR=build/

SOURCE=skat.c server.c connection.c atomic_queue.c card.c player.c
OBJS=$(SOURCE:.c=.o)
HEADERS=$(SOURCE:.c=.h)
EXTRA_HEADERS=event.h action.h
EXTRA_SOURCE=main_client.c main_server.c

.PHONY: default clean png_gone distclean png

default: all

all: server client

server: $(OBJS) main_server.o 
	$(CC) $(CFLAGS) -o $@ $^

client: $(OBJS) main_client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o:%.c $(HEADERS) $(EXTRA_HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJS) main_server.o main_client.o dep_graph.png

distclean:
	$(RM) $(OBJS) main_server.o main_client.o dep_graph.png
	$(RM) server client

force_rebuild: | distclean all

format: $(SOURCE) $(HEADERS) $(EXTRA_HEADERS) $(EXTRA_SOURCE)
	clang-format -i $^

png_gone:
	$(RM) dep_graph.png

png:
	./dep_graph.sh
