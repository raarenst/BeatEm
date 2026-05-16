
# Windows (MinGW-w64) build configuration.
#
# WARNING: this build is currently broken. After the Phase-3 transport
# migration, source/ws.c, source/beatem_server.c, and
# source/beatem_client.c call POSIX socket APIs directly (recv/send/
# accept, struct sockaddr_in, <pthread.h>, <sys/select.h>, <netdb.h>).
# A Windows port needs:
#   - winsock2 replacements for recv/send/accept/socket/connect/select,
#     WSAStartup/WSACleanup, and SOCKET_ERROR/WSAEINTR handling in
#     ws.c's read_exact/write_exact;
#   - a pthread shim (the MinGW pthreads-win32 package works, or just
#     swap to native CreateThread + CRITICAL_SECTION/CONDITION_VARIABLE);
#   - <unistd.h> stand-ins for close()/sleep_ms().
#
# Until that port lands, only the Linux build (../build_linux/makefile)
# is supported.

# ----- Tools -----
CC	= "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe"
LINKER	= "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe"

# ----- Folders -----
INCDIR	:= ../include
SRCDIR	:= ../source
OBJDIR	:= obj
BINDIR	:= .

# Path to a MinGW-built libsodium with include/sodium.h and lib/libsodium.a
LIBSODIUM_DIR	?= C:/libsodium

# ----- Files -----
EXE        := $(BINDIR)/beatem_server.exe
EXE_CLIENT := $(BINDIR)/beatem_client.exe

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
CPPFLAGS := -I$(INCDIR) -I$(LIBSODIUM_DIR)/include -MMD -MP
CFLAGS   := -Wall
LDFLAGS  := -L$(LIBSODIUM_DIR)/lib
LDLIBS   := -lm -lws2_32 -lwsock32 -lsodium

.PHONY: all clean

all: $(EXE) $(EXE_CLIENT)

$(EXE): $(OBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE_CLIENT): $(CLIENT_OBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BINDIR) $(OBJDIR):
	mkdir $@

clean:
	-del /Q "$(subst /,\,$(OBJDIR))\*.o" "$(subst /,\,$(OBJDIR))\*.d" 2>nul
	-del /Q "$(subst /,\,$(EXE))" "$(subst /,\,$(EXE_CLIENT))" 2>nul

-include $(OBJ:.o=.d)
-include $(CLIENT_OBJ:.o=.d)
