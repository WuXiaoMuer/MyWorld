# MyWorld - Standard Makefile for VS Code / GNU Make
# For Red Panda C++, use makefile.win instead

CC       = gcc
CXX      = g++
WINDRES  = windres

INCS     = -I./include
CFLAGS   = $(INCS) -O2 -pipe -mwindows
LIBS     = -L./lib -Wl,--stack,12582912 -s -lraylib -lopengl32 -lgdi32 -lwinmm -static

SRCS     = main.c noise.c world.c player.c daynight.c rendering.c save.c game.c crafting.c
OBJS     = $(SRCS:.c=.o)
RES      = MyWorld_private.res
BIN      = MyWorld.exe

all: $(BIN)

$(BIN): $(OBJS) $(RES)
	$(CXX) $(OBJS) $(RES) -o $@ $(LIBS)

%.o: %.c types.h
	$(CC) -c $< -o $@ $(CFLAGS)

$(RES): MyWorld_private.rc
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff

clean:
	-rm -f $(OBJS) $(RES) $(BIN)

.PHONY: all clean
