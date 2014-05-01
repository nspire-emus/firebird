SRCS = $(wildcard *.S *.c */*.c)
OBJS = $(patsubst %.S, %.o, $(SRCS:.c=.o))
EXEC = nspire_emu
OBJCOPY = objcopy
CC = gcc
WINDRES = windres
CFLAGS = -W -Wall -m32
LDFLAGS = -L/usr/lib32
ifeq ($(OS),Windows_NT)
	OBJS += resource.o
	LDFLAGS += -mi386pe --no-leading-underscore
	CFLAGS += -fno-leading-underscore
	OBJCOPYFLAGS = --remove-leading-char
else
	LDFLAGS += -melf_i386
endif

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

resource.o: gui/gui-windows-resource.rc gui/gui-windows-id.h
	$(WINDRES) $< -o $@

armsnippets.o: armsnippets.S
	arm-none-eabi-gcc -fno-leading-underscore -c $< -o $@ -mcpu=arm7tdmi
	arm-none-eabi-objcopy $(OBJCOPYFLAGS) -O binary $@ snippets.bin
	$(LD) $(LDFLAGS) -r -b binary -o $@ snippets.bin
	rm snippets.bin
	$(OBJCOPY) $(OBJCOPYFLAGS) --rename-section .data=.rodata,alloc,load,readonly,data,contents $@

%.o: %.S
	$(CC) -c $< -o $@ -m32

%.o: %.c
	$(CC) $(CFLAGS) -Os -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
