UNAME_S := $(shell uname -s)
CC := gcc
# Model compiler
MC := ./tetgen
MCFLAGS := -pYBNI
CFLAGS := -Iinclude -c -g -DGLFW_INCLUDE_NONE -Wall -Wextra -Werror -Wno-int-in-bool-context -Wno-misleading-indentation -Wno-shift-negative-value
CPPFLAGS := -std=c++11
ifeq ($(UNAME_S), Linux)
	LDFLAGS := -lstdc++ -lm -lglfw
endif
ifeq ($(findstring MSYS, $(UNAME_S)), MSYS)
	LDFLAGS := -Llib -lglfw3dll -lgdi32 -lstdc++
endif
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
OUTDIR := bin
DLLDIR := deploy
EXEC_NAME := $(OUTDIR)/PRIM
RESDIR := res
SRCDIR := src
DEPDIR := deps
OBJDIR := obj
SOURCES := $(wildcard $(SRCDIR)/*.c*)
MODELS := $(wildcard $(RESDIR)/models/*.off)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o, $(SOURCES))
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o, $(OBJS))
TETRAS := $(patsubst %.off,%.ele, $(MODELS))

.PHONY: all clean run $(RESDIR)

all: $(DEPDIR) $(OBJDIR) $(OUTDIR) $(EXEC_NAME) $(TETRAS)
	@cp -r $(DLLDIR)/* $(OUTDIR)
	@[ "$(shell ls -A $(RESDIR))" ] && cp -r $(RESDIR)/* $(OUTDIR) || :

clean:
	rm -f $(EXEC_NAME)
	rm -rf $(OUTDIR)
	rm -rf $(OBJDIR)
	rm -rf $(DEPDIR)
	rm -f $(TETRAS) $(patsubst %.off,%.face, $(MODELS))

run: all
	@echo ">>> Running $(EXEC_NAME) ..."
	@$(EXEC_NAME)

$(DEPDIR):
	@mkdir $(DEPDIR)

$(OUTDIR):
	@mkdir $(OUTDIR)

$(OBJDIR):
	@mkdir $(OBJDIR)

$(EXEC_NAME): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

-include $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$(OBJS))

%.ele: %.off
	$(MC) $(MCFLAGS) $<
	@rm -f $*.smesh $*.edge

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $< -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) $< -o $@
