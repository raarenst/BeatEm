
# ----- Tools -----
CC	= gcc
LINKER	= ld
rm 	= rm

ARCH	:= linux

# ----- Folders -----
INCDIR	:= ../include
SRCDIR	:= ../source
LIBDIR	:= ../lib
OBJDIR	:= obj
BINDIR	:= .
BABEL_ARCH	:= ../babel/source/$(ARCH)
BABEL_INCLUDE	:= ../babel/include

# ----- Files -----
EXE := $(BINDIR)/beatem_server
EXE_CLIENT := $(BINDIR)/beatem_client
EXE_KEYGEN := $(BINDIR)/beatem_keygen
EXE_TEST_UNIT := $(BINDIR)/test_unit
TESTDIR := ../tests

SRC := ../source/beatem_server.c
SRC += ../source/crypto.c
SRC += ../source/protocol.c
CLIENT_SRC := ../source/beatem_client.c
CLIENT_SRC += ../source/crypto.c
CLIENT_SRC += ../source/protocol.c

BSRC := $(BABEL_ARCH)/babelsock.c
BSRC += $(BABEL_ARCH)/babelthread.c
BSRC += $(BABEL_ARCH)/babeltime.c


#SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
BOBJ := $(BSRC:$(BABEL_ARCH)/%.c=$(OBJDIR)/%.o)
CLIENT_OBJ := $(CLIENT_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

RMOBJ := $(subst /,\,$(OBJ)) # needed for windows del
RMEXE := $(subst /,\,$(EXE)) # needed for windows del



# ----- Flags -----
CPPFLAGS := -I$(INCDIR) -I$(BABEL_INCLUDE) -MMD -MP
CFLAGS	 := -Wall -pthread
LDFLAGS  := -Llib 
LDLIBS   := -lm -lpthread -lsodium

.PHONY: all clean remove test test-unit test-smoke

all: $(EXE) $(EXE_CLIENT)

# Build + run all tests
test: test-unit test-smoke

# Unit tests link against crypto.o + protocol.o (no babel, no app code)
test-unit: $(EXE_TEST_UNIT)
	@./$(EXE_TEST_UNIT)

$(EXE_TEST_UNIT): $(TESTDIR)/test_unit.c $(OBJDIR)/crypto.o $(OBJDIR)/protocol.o | $(BINDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Smoke test: runs the server + 2 clients via the shell script
test-smoke: $(EXE) $(EXE_CLIENT) $(EXE_KEYGEN)
	@bash $(TESTDIR)/smoke_test.sh

$(EXE_KEYGEN): $(TESTDIR)/beatem_keygen.c | $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@


disp:
	@echo $(RMOBJ)

$(EXE): $(OBJ) $(BOBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE_CLIENT): $(CLIENT_OBJ) $(BOBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o:	$(BABEL_ARCH)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o:	$(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BINDIR) $(OBJDIR):
	mkdir $@

clean:
	@$(rm) $(RMOBJ) $(RMOBJ:.o=.d)
	@echo "Cleanup complete!"

remove: clean
	@$(rm) $(RMEXE)
	@echo "Executable removed!"

-include $(OBJ:.o=.d)
