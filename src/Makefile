CFLAGS = -W -Wall -m32
LDFLAGS= -m32 -L/usr/lib32
SRCS = $(wildcard *.S *.c */*.c)
OBJS = $(patsubst %.S, %.o, $(SRCS:.c=.o))
EXEC = nspire_emu
OBJCOPY = objcopy
CC = gcc

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

armsnippets.o: armsnippets.S
	arm-none-eabi-gcc -c $< -o $@ -mcpu=arm7tdmi
	arm-none-eabi-objcopy -O binary $@ snippets.bin
	$(LD) -melf_i386 -r -b binary -o armsnippets.o snippets.bin
	rm snippets.bin
	$(OBJCOPY) --rename-section .data=.rodata,alloc,load,readonly,data,contents armsnippets.o armsnippets.o

%.o: %.S
	$(CC) -c $< -o $@ -m32

%.o: %.c
	$(CC) $(CFLAGS) -Os -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
