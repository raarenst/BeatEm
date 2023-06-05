
# ----- Tools -----
CC	= "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe"
LINKER	= "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe"
rm 	= del

ARCH	:= win64

# ----- Folders -----
INCDIR	:= ../include
SRCDIR	:= ../source
LIBDIR	:= ../lib
OBJDIR	:= obj
BINDIR	:= .
BABEL_ARCH	:= ../babel/source/$(ARCH)
BABEL_INCLUDE	:= ../babel/include

# ----- Files -----
EXE := $(BINDIR)/bruno_server.exe
EXE_CLIENT := $(BINDIR)/bruno_client.exe

SRC := ../source/bruno_server.c
SRC += ../source/crypto.c
CLIENT_SRC := ../source/bruno_client.c
CLIENT_SRC += ../source/crypto.c

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
CFLAGS	 := -Wall
LDFLAGS  := -Llib 
LDLIBS   := -lm -lws2_32 -lwsock32

.PHONY: all clean remove

all: $(EXE) $(EXE_CLIENT) 

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
