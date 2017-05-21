#FLAGS := -g -O3 -flto -fno-fat-lto-objects -DBENCHMARK -Wall -I.. -fuse-ld=gold
FLAGS := -g -DBENCHMARK -Wall -I..
#FLAGS := -DBENCHMARK -Wall -I.. -fuse-ld=gold
CFLAGS := -std=c11 $(FLAGS)
CXXFLAGS := -std=c++11 $(FLAGS)
LFLAGS := -lz
OUTPUT := firebird-headless

ASMSOURCES := ../core/asmcode_aarch64.S

CSOURCES   := ../core/armsnippets_loader.c ../core/asmcode.c ../core/casplus.c ../core/des.c ../core/disasm.c \
	      ../core/gdbstub.c ../core/interrupt.c ../core/keypad.c ../core/lcd.c ../core/link.c ../core/mem.c \
	      ../core/misc.c ../core/mmu.c ../core/schedule.c ../core/serial.c ../core/sha256.c ../core/usb.c \
              ../core/usblink.c ../core/os/os-linux.c

CPPSOURCES := ../core/arm_interpreter.cpp ../core/coproc.cpp ../core/cpu.cpp ../core/debug.cpp ../core/emu.cpp \
	      ../core/flash.cpp ../core/gif.cpp ../core/thumb_interpreter.cpp ../core/usblink_queue.cpp main.cpp \
	      ../core/translate_aarch64.cpp

OBJS = $(patsubst %.S, %.o, $(ASMSOURCES))
OBJS += $(patsubst %.c, %.o, $(CSOURCES))
OBJS += $(patsubst %.cpp, %.o, $(CPPSOURCES))

all: $(OUTPUT)

%.o: %.S Makefile
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp Makefile
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(OUTPUT): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(OUTPUT)
