
# ----- Tools -----
CC	= gcc

# ----- Folders -----
INCDIR	:= ../include
SRCDIR	:= ../source
OBJDIR	:= obj
BINDIR	:= .
TESTDIR := ../tests

# ----- Files -----
EXE           := $(BINDIR)/beatem_server
EXE_CLIENT    := $(BINDIR)/beatem_client
EXE_KEYGEN    := $(BINDIR)/beatem_keygen
EXE_TEST_UNIT := $(BINDIR)/test_unit

SRC := $(SRCDIR)/beatem_server.c
SRC += $(SRCDIR)/crypto.c
SRC += $(SRCDIR)/protocol.c
SRC += $(SRCDIR)/ws.c
CLIENT_SRC := $(SRCDIR)/beatem_client.c
CLIENT_SRC += $(SRCDIR)/crypto.c
CLIENT_SRC += $(SRCDIR)/protocol.c
CLIENT_SRC += $(SRCDIR)/ws.c

OBJ        := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CLIENT_OBJ := $(CLIENT_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# ----- Flags -----
CPPFLAGS := -I$(INCDIR) -MMD -MP
CFLAGS   := -Wall -pthread
LDFLAGS  := -Llib
LDLIBS   := -lm -lpthread -lsodium

.PHONY: all clean test test-unit test-smoke

all: $(EXE) $(EXE_CLIENT)

# ----- Tests -----
test: test-unit test-smoke

# Unit tests link against crypto.o, protocol.o, ws.o — no app code.
test-unit: $(EXE_TEST_UNIT)
	@./$(EXE_TEST_UNIT)

$(EXE_TEST_UNIT): $(TESTDIR)/test_unit.c $(OBJDIR)/crypto.o $(OBJDIR)/protocol.o $(OBJDIR)/ws.o | $(BINDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Smoke test: server + 2 clients via the shell script.
test-smoke: $(EXE) $(EXE_CLIENT) $(EXE_KEYGEN)
	@bash $(TESTDIR)/smoke_test.sh

$(EXE_KEYGEN): $(TESTDIR)/beatem_keygen.c | $(BINDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

# ----- Build rules -----
$(EXE): $(OBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE_CLIENT): $(CLIENT_OBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BINDIR) $(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR) \
	       $(EXE) $(EXE_CLIENT) $(EXE_KEYGEN) $(EXE_TEST_UNIT)

-include $(OBJ:.o=.d)
-include $(CLIENT_OBJ:.o=.d)
