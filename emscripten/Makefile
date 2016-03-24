CC := emcc
CXX := em++
FLAGS := -O3 --llvm-lto 3 --closure 1 -Wall -I.. -DNO_SETJMP -s USE_ZLIB=1
CFLAGS := -std=c11 $(FLAGS)
CXXFLAGS := -std=c++11 $(FLAGS)
LFLAGS := --emrun -s TOTAL_MEMORY=500000000
OUTPUT := firebird

CSOURCES :=    ../core/armsnippets_loader.c ../core/asmcode.c ../core/casplus.c ../core/des.c ../core/disasm.c \
	      ../core/gdbstub.c ../core/interrupt.c ../core/keypad.c ../core/lcd.c ../core/link.c ../core/mem.c \
	      ../core/misc.c ../core/mmu.c ../core/schedule.c ../core/serial.c ../core/sha256.c ../core/usb.c \
              ../core/usblink.c ../core/os/os-emscripten.c

CPPSOURCES := ../core/arm_interpreter.cpp ../core/coproc.cpp ../core/cpu.cpp ../core/debug.cpp ../core/emu.cpp \
	      ../core/flash.cpp ../core/gif.cpp ../core/thumb_interpreter.cpp ../core/usblink_queue.cpp main.cpp

OBJS = $(patsubst %.c, %.bc, $(CSOURCES))
OBJS += $(patsubst %.cpp, %.bc, $(CPPSOURCES))

all: $(OUTPUT).js

%.bc: %.c Makefile
	$(CC) $(CFLAGS) $< -o $@

%.bc: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

test: $(OUTPUT).js
	emrun --safe_firefox_profile --browser firefox $(OUTPUT).html boot1.img

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@ --preload-file boot1.img

clean:
	rm -f $(OBJS) $(OUTPUT).js $(OUTPUT).data $(OUTPUT).js.mem
