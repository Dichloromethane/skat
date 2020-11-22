# sooooo beautiful
# ahhhh

CC=gcc

WARNINGS=-Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -Wfatal-errors

CPPFLAGS=-MMD -MP -pthread $(INCLUDEDIR_FLAGS) -D _GNU_SOURCE # -I /usr/include/libpng16
#CFLAGS=-O3 -ftree-vectorize -mcpu=native -mtune=native -flto $(EXTRA_CFLAGS)
CFLAGS=-std=gnu2x -O0 -ggdb3 -fasynchronous-unwind-tables #-fsanitize=address

LDFLAGS=

LDLIBS=-pthread -lrt -lc
LDLIBS_SERVER=$(LDLIBS)
LDLIBS_CLIENT=$(LDLIBS) -lglfw -lGL -ldl -lfreetype -lm # -lGLU -lX11 -lXrandr -lXi -lpng16 -lz

TOOLSDIR=tools/

INCLUDEDIR=include/ conf/
SKAT_INCLUDEDIR=include/skat/
SERVER_INCLUDEDIR=include/server/
CLIENT_INCLUDEDIR=include/client/

XMACROSDIR=defs/

SYSTEM_INCLUDES=/usr/include/freetype2
INCLUDEDIR_FLAGS=$(addprefix -I,$(INCLUDEDIR) $(XMACROSDIR) $(SYSTEM_INCLUDES))

SOURCEDIR=src/
SKAT_SOURCEDIR=$(SOURCEDIR)skat/
SERVER_SOURCEDIR=$(SOURCEDIR)server/
CLIENT_SOURCEDIR=$(SOURCEDIR)client/

# All build directories will be deleted when executing "clean".
# Do NOT set this to the same directory as your code!
BUILDDIR=build/
SKAT_BUILDDIR=$(BUILDDIR)skat/
SERVER_BUILDDIR=$(BUILDDIR)server/
CLIENT_BUILDDIR=$(BUILDDIR)client/
BUILDDIRS=$(SKAT_BUILDDIR) $(SERVER_BUILDDIR) $(CLIENT_BUILDDIR) $(BUILDDIR)
COMP_COMMANDS=compile_commands.json
BEAR_REBUILD_FILE=$(BUILDDIR)bear_sources

SKAT_SOURCE=$(wildcard $(SKAT_SOURCEDIR)*.c)
SERVER_SOURCE=$(wildcard $(SERVER_SOURCEDIR)*.c)
CLIENT_SOURCE=$(wildcard $(CLIENT_SOURCEDIR)*.c)
SOURCE=$(SKAT_SOURCE) $(SERVER_SOURCE) $(CLIENT_SOURCE)

HEADER=$(wildcard $(addsuffix *.h,$(INCLUDEDIR))) $(wildcard $(SKAT_INCLUDEDIR)*.h) $(wildcard $(SERVER_INCLUDEDIR)*.h) $(wildcard $(CLIENT_INCLUDEDIR)*.h)
XMACROS=$(wildcard $(XMACROSDIR)*.def)

EVERYTHING=$(SOURCE) $(HEADER) $(XMACROS)

SKAT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SKAT_SOURCE:.c=.o))
SERVER_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SERVER_SOURCE:.c=.o))
CLIENT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(CLIENT_SOURCE:.c=.o))
OBJ=$(SKAT_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)

DEP=$(OBJ:.o=.d)

REBUILDING_MARKER=$(BUILDDIR).rebuilding_marker
REBUILDING_RULE=$(BUILDDIR).rebuilding_rule_marker
ARTIFICIAL=$(REBUILDING_RULE) $(REBUILDING_MARKER)

.PHONY: default all png clean distclean bear all_ cond

default: all

all: cond

NEW_SOURCES:=$(filter-out $(file <$(BEAR_REBUILD_FILE)), $(EVERYTHING))
REMOVED_SOURCES:=$(filter-out $(EVERYTHING), $(file <$(BEAR_REBUILD_FILE)))
CHANGED_SOURCES:=$(NEW_SOURCES)$(REMOVED_SOURCES)
BEAR_TARGET:=$(if $(CHANGED_SOURCES), with_new_files, without_new_files)

cond: $(BEAR_TARGET)

with_new_files: bear
	echo "$(EVERYTHING)" > $(BEAR_REBUILD_FILE)
without_new_files: comp_

all_: skat_server skat_client

skat_server: $(SKAT_OBJ) $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_SERVER) -o $@

skat_client: $(SKAT_OBJ) $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_CLIENT) -o $@

$(OBJ): $(BUILDDIR)%.o: $(SOURCEDIR)%.c Makefile | $(BUILDDIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -o $@ -c $<

$(BUILDDIRS):
	mkdir -p $@

.DELETE_ON_ERROR: $(ARTIFICIAL)

comp_:: $(COMP_COMMANDS)
comp_:: $(REBUILDING_RULE)
comp_:: delete_marker

$(COMP_COMMANDS): Makefile
	$(MAKE) bear
	touch $(REBUILDING_MARKER)
	touch $(REBUILDING_RULE)
$(REBUILDING_RULE): $(REBUILDING_MARKER)
	$(MAKE) all_
$(REBUILDING_MARKER):
	touch $(REBUILDING_MARKER)
delete_marker:
	$(RM) $(REBUILDING_RULE) $(REBUILDING_MARKER)

bear:: clean
bear:: | $(BUILDDIRS)
	bear -o $(COMP_COMMANDS) $(MAKE) all_

clean:
	$(RM) $(DEP) $(OBJ)
	$(RM) $(COMP_COMMANDS)
	$(RM) $(ARTIFICIAL)

distclean: clean
	$(RM) -r $(BUILDDIRS)
	$(RM) dep_graph.png
	$(RM) skat_server skat_client

format: $(SOURCE) $(HEADER) $(XMACROS)
	clang-format -i $^

ctags: $(SOURCE) $(HEADER) $(XMACROS)
	ctags --extra=+f --c-kinds=+p --tag-relative=yes -o .tags -R $^

png:
	./$(TOOLSDIR)dep_graph.sh -o dep_graph.png -s $(SOURCEDIR) -s $(INCLUDEDIR) -s $(XMACROSDIR)

-include $(DEP)
