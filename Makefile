CC=gcc

WARNINGS=-Wall -Wextra -Wfatal-errors -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas

#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto $(WARNINGS)
CFLAGS=-pthread -O0 -ggdb3

LIBS=-lrt -lc

BUILDDIR=build/
TOOLSDIR=tools/
SOURCEDIR=src/
INCLUDEDIR=include/

SOURCE=skat.c server.c connection.c atomic_queue.c card.c card_collection.c stich.c player.c util.c ctimer.c
EXTRA_SOURCE=skat_server_main.c

SOURCE_WITH_DIR=$(addprefix $(SOURCEDIR), $(SOURCE) $(EXTRA_SOURCE))

OBJS=$(addprefix $(BUILDDIR), $(SOURCE:.c=.o))
EXTRA_OBJS=$(addprefix $(BUILDDIR), $(EXTRA_SOURCE:.c=.o))

HEADERS=$(SOURCE:.c=.h)
EXTRA_HEADERS=event.h action.h game_rules.h
ALL_HEADERS=$(addprefix $(INCLUDEDIR), $(HEADERS) $(EXTRA_HEADERS))



.PHONY: default clean png_gone distclean png all skat

default: all

all: skat_server

skat_server: | $(BUILDDIR) skat_server.elf2

skat_server.elf2: $(OBJS) $(BUILDDIR)skat_server_main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(EXTRA_OBJS):$(BUILDDIR)%.o:$(SOURCEDIR)%.c $(ALL_HEADERS)
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) $(WARNINGS) -c -o $@ $<

$(OBJS):$(BUILDDIR)%.o:$(SOURCEDIR)%.c $(ALL_HEADERS)
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) $(WARNINGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $@

clean:
	$(RM) $(OBJS) $(EXTRA_OBJS)

distclean: clean png_gone
	rmdir $(BUILDDIR)
	$(RM) skat_server.elf2

force_rebuild: | distclean all

format: $(SOURCE_WITH_DIR) $(ALL_HEADERS)
	clang-format -i $^

png_gone:
	$(RM) dep_graph.png

png:
	./$(TOOLSDIR)dep_graph.sh -s $(SOURCEDIR) -s $(INCLUDEDIR)
