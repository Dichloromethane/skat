CC=gcc

WARNINGS=-Wall -Wextra -Wfatal-errors -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas

#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto $(WARNINGS)
CFLAGS=-pthread -O0 -ggdb3 $(WARNINGS)

BUILDDIR=build/
TOOLSDIR=tools/
SOURCEDIR=src/
INCLUDEDIR=include/

SOURCE=skat.c server.c connection.c atomic_queue.c card.c card_collection.c stich.c player.c util.c
EXTRA_SOURCE=main.c

SOURCE_WITH_DIR=$(addprefix $(SOURCEDIR), $(SOURCE) $(EXTRA_SOURCE))

OBJS=$(addprefix $(BUILDDIR), $(SOURCE:.c=.o))
EXTRA_OBJS=$(addprefix $(BUILDDIR), $(EXTRA_SOURCE:.c=.o))

HEADERS=$(SOURCE:.c=.h)
EXTRA_HEADERS=event.h action.h game_rules.h
ALL_HEADERS=$(addprefix $(INCLUDEDIR), $(HEADERS) $(EXTRA_HEADERS))


.PHONY: default clean png_gone distclean png all skat

default: all

all: skat

skat: | $(BUILDDIR) skat.elf2

skat.elf2: $(OBJS) $(BUILDDIR)main.o
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
	$(RM) -r $(BUILDDIR)
	$(RM) skat.elf2

force_rebuild: | distclean all

format: $(SOURCE_WITH_DIR) $(ALL_HEADERS)
	clang-format -i $^

png_gone:
	$(RM) dep_graph.png

png:
	./$(TOOLSDIR)dep_graph.sh -s $(SOURCEDIR) -s $(INCLUDEDIR)
