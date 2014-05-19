SRCS = $(wildcard *.S *.c */*.c)
OBJS = $(patsubst %.S, %.o, $(SRCS:.c=.o))
EXEC = nspire_emu
OBJCOPY = objcopy
CC = gcc
WINDRES = windres
CFLAGS = -W -Wall -m32 -ggdb
LDFLAGS = -L/usr/lib32
ifeq ($(OS),Windows_NT)
	OBJS += resource.o
	LDFLAGS += -mi386pe
	LIBS += -lgdi32 -lcomdlg32 -lwinmm -lws2_32
else
	LDFLAGS += -melf_i386
endif

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LIBS) -m32

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
