CC=gcc

WARNINGS=-Wall -Wextra -Wfatal-errors -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas

CPPFLAGS=-MMD -pthread -I $(INCLUDEDIR) -I /usr/include/freetype2 # -I /usr/include/libpng16
#CFLAGS=-Wall -O3 -mcpu=native -pthread -flto
CFLAGS=-O0 -ggdb3 

LDFLAGS=

LDLIBS=-pthread -lrt -lc
LDLIBS_SERVER=$(LDLIBS)
LDLIBS_CLIENT=$(LDLIBS) -lglfw -lGL -ldl -lfreetype # -lX11 -lXrandr -lXi -lm  -lpng16 -lz

TOOLSDIR=tools/

INCLUDEDIR=include/

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
HEADER=$(wildcard $(SKAT_SOURCEDIR)*.h) $(wildcard $(SERVER_SOURCEDIR)*.h) $(wildcard $(CLIENT_SOURCEDIR)*.h)

SKAT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SKAT_SOURCE:.c=.o))
SERVER_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(SERVER_SOURCE:.c=.o))
CLIENT_OBJ=$(patsubst $(SOURCEDIR)%,$(BUILDDIR)%,$(CLIENT_SOURCE:.c=.o))
OBJ=$(SKAT_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)

DEP=$(OBJ:.o=.d)

.PHONY: default all png clean

default: all

all: skat_server skat_client

skat_server: $(SKAT_OBJ) $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_SERVER) -o $@

skat_client: $(SKAT_OBJ) $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS_CLIENT) -o $@

$(OBJ): $(BUILDDIR)%.o: $(SOURCEDIR)%.c | $(BUILDDIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -o $@ -c $<

$(BUILDDIRS):
	mkdir -p $@

distclean: clean
	$(RM) skat_server skat_client 

clean:
	$(RM) $(DEP) $(OBJ) dep_graph.png
	-rmdir $(BUILDDIRS)

format: $(SOURCE) $(ALL_HEADERS)
	clang-format -i $^

png:
	./$(TOOLSDIR)dep_graph.sh -o dep_graph.png -s $(SOURCEDIR) -s $(INCLUDEDIR)

-include $(DEP)
