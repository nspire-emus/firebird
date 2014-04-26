FLAGS = -W -Wall
SRCS = $(wildcard *.S *.c */*.c)
OBJS = $(patsubst %.S, %.o, $(SRCS:.c=.o))
EXEC = nspire_emu

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@

armsnippets.o: armsnippets.S
	arm-none-eabi-gcc -c $< -o $@ -mcpu=arm7tdmi
	arm-none-eabi-objcopy -O binary $@ snippets.bin
	$(LD) -r -b binary -o armsnippets.o snippets.bin
	rm snippets.bin
	$(OBJCOPY) --rename-section .data=.rodata,alloc,load,readonly,data,contents armsnippets.o armsnippets.o

%.o: %.S
	$(CC) -c $< -o $@ -m32

%.o: %.c
	$(CC) $(FLAGS) -Os -c $< -o $@

clean:
	rm -r $(OBJS) $(EXEC)
