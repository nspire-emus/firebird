SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
EXEC = nspire_emu

.PHONY: all

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@
