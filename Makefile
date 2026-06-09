# MyWorld - Standard Makefile for VS Code / GNU Make
# For Red Panda C++, use makefile.win instead

CC       = gcc
CXX      = g++
WINDRES  = windres

INCS     = -I./include
CFLAGS   = $(INCS) -MMD -MP -O2 -pipe -mwindows
LIBS     = -L./lib -Wl,--stack,12582912 -s -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 -static

BUILDDIR = build
SRCS     = main.c noise.c world.c player.c daynight.c rendering.c save.c game.c crafting.c sound.c mob.c light.c particles.c entities.c i18n.c weather.c net.c
OBJS     = $(SRCS:%.c=$(BUILDDIR)/%.o)
DEPS     = $(OBJS:%.o=%.d)
RES      = $(BUILDDIR)/MyWorld_private.res
BIN      = MyWorld.exe

all: $(BIN)

$(BIN): $(OBJS) $(RES)
	$(CXX) $(OBJS) $(RES) -o $@ $(LIBS)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILDDIR)/sound.o: sound.c sound.h | $(BUILDDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILDDIR)/net.o: net.c net.h | $(BUILDDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILDDIR)/MyWorld_private.res: MyWorld_private.rc | $(BUILDDIR)
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	-rm -rf $(BUILDDIR) $(BIN) *.o

-include $(DEPS)

.PHONY: all clean
