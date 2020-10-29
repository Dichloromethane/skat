# sooooo beautiful
# ahhhh

CC=gcc

WARNINGS=-Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas

CPPFLAGS=-MMD -MP -pthread $(INCLUDEDIR_FLAGS) -D _GNU_SOURCE # -I /usr/include/libpng16
#CFLAGS=-O3 -ftree-vectorize -mcpu=native -mtune=native -flto $(EXTRA_CFLAGS)
CFLAGS=-std=gnu17 -O0 -ggdb3 -flto #-fsanitize=address

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

BUILDDIR=build/
SKAT_BUILDDIR=$(BUILDDIR)skat/
SERVER_BUILDDIR=$(BUILDDIR)server/
CLIENT_BUILDDIR=$(BUILDDIR)client/
BUILDDIRS=$(SKAT_BUILDDIR) $(SERVER_BUILDDIR) $(CLIENT_BUILDDIR) $(BUILDDIR)

SKAT_SOURCE=$(wildcard $(SKAT_SOURCEDIR)*.c)
SERVER_SOURCE=$(wildcard $(SERVER_SOURCEDIR)*.c)
CLIENT_SOURCE=$(wildcard $(CLIENT_SOURCEDIR)*.c)
SOURCE=$(SKAT_SOURCE) $(SERVER_SOURCE) $(CLIENT_SOURCE)

HEADER=$(wildcard $(addsuffix *.h,$(INCLUDEDIR))) $(wildcard $(SKAT_INCLUDEDIR)*.h) $(wildcard $(SERVER_INCLUDEDIR)*.h) $(wildcard $(CLIENT_INCLUDEDIR)*.h)
XMACROS=$(wildcard $(XMACROSDIR)*.def)

SKAT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SKAT_SOURCE:.c=.o))
SERVER_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SERVER_SOURCE:.c=.o))
CLIENT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(CLIENT_SOURCE:.c=.o))
OBJ=$(SKAT_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)

DEP=$(OBJ:.o=.d)

 
.PHONY: default all png clean distclean

default: all 

all: skat_server skat_client

skat_server: $(SKAT_OBJ) $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_SERVER) -o $@

skat_client: $(SKAT_OBJ) $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_CLIENT) -o $@

$(OBJ): $(BUILDDIR)%.o: $(SOURCEDIR)%.c Makefile | $(BUILDDIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -o $@ -c $<

$(BUILDDIRS):
	mkdir -p $@

distclean: clean
	$(RM) skat_server skat_client

clean:
	$(RM) $(DEP) $(OBJ) dep_graph.png
	-rmdir $(BUILDDIRS)

format: $(SOURCE) $(HEADER) $(XMACROS)
	clang-format -i $^

ctags: $(SOURCE) $(HEADER) $(XMACROS)
	ctags --extra=+f --c-kinds=+p --tag-relative=yes -o .tags -R $^

png:
	./$(TOOLSDIR)dep_graph.sh -o dep_graph.png -s $(SOURCEDIR) -s $(INCLUDEDIR)

-include $(DEP)
