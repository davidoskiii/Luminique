CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -Wno-unused-parameter
LDFLAGS = -lm

SRCDIR = .
OBJDIR = Make/obj
BINDIR = Make/bin

SRCS := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = $(BINDIR)/luminique

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)
