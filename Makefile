CC=gcc

WARNINGS=-Wall -Wextra -Wfatal-errors -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas

#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto $(WARNINGS)
CFLAGS=-pthread -O0 -ggdb3 $(WARNINGS)

BUILDDIR=build/
TOOLSDIR=tools/
SOURCEDIR=src/
INCLUDEDIR=$(SOURCEDIR)include/

SOURCE=skat.c server.c connection.c atomic_queue.c card.c card_collection.c stich.c player.c util.c
EXTRA_SOURCE=main_client.c main_server.c

OBJS=$(addprefix $(BUILDDIR), $(SOURCE:.c=.o))
EXTRA_OBJS=$(addprefix $(BUILDDIR), $(EXTRA_SOURCE:.c=.o))

HEADERS=$(SOURCE:.c=.h)
EXTRA_HEADERS=event.h action.h game_rules.h
ALL_HEADERS=$(addprefix $(INCLUDEDIR), $(HEADERS) $(EXTRA_HEADERS))


.PHONY: default clean png_gone distclean png all server client

default: all

all: server client

server: | $(BUILDDIR) server.elf2

client: | $(BUILDDIR) client.elf2

server.elf2: $(OBJS) $(BUILDDIR)main_server.o 
	$(CC) $(CFLAGS) -o $@ $^

client.elf2: $(OBJS) $(BUILDDIR)main_client.o
	$(CC) $(CFLAGS) -o $@ $^

$(EXTRA_OBJS):$(BUILDDIR)%.o:$(SOURCEDIR)%.c $(ALL_HEADERS)
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c -o $@ $<

$(OBJS):$(BUILDDIR)%.o:$(SOURCEDIR)%.c $(ALL_HEADERS)
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $@

clean:
	$(RM) $(OBJS) $(EXTRA_OBJS)

distclean: clean png_gone
	rmdir $(BUILDDIR)
	$(RM) server.elf2 client.elf2

force_rebuild: | distclean all

format: $(SOURCE) $(HEADERS) $(EXTRA_HEADERS) $(EXTRA_SOURCE)
	clang-format -i $^

png_gone:
	$(RM) dep_graph.png

png:
	./$(TOOLDIR)dep_graph.sh -o ./dep_graph.png
