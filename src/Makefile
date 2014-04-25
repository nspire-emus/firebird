SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
EXEC = nspire_emu

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@

clean:
	rm -r $(OBJS)
