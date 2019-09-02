FLAGS := -g -O3 -DBENCHMARK -Wall -I..
OUTPUT := firebird-headless

TOOLCHAIN_ARCH = $(shell LANG=C $(CC) -v 2>&1 | awk '/Target:(.*)/{ print $$2 }' | cut -d- -f1)
TRANSLATION_ENABLED ?= TRUE
SUPPORT_LINUX ?= TRUE

ifeq "$(TOOLCHAIN_ARCH)" ""
    $(error Could not determine the toolchain architecture, please specify manually)
endif

ifeq "$(TRANSLATION_ENABLED)" "TRUE"
    ifneq "$(filter i%86,$(TOOLCHAIN_ARCH))" ""
        ASMSOURCES += ../core/asmcode_x86.S
        CSOURCES += ../core/translation_x86.c
    else ifeq "$(TOOLCHAIN_ARCH)" "x86_64"
        ASMSOURCES += ../core/asmcode_x86_64.S
        CSOURCES += ../core/asmcode.c ../core/translate_x86_64.c
    else ifneq "$(filter arm%,$(TOOLCHAIN_ARCH))" ""
        ASMSOURCES += ../core/asmcode_arm.S
        CSOURCES += ../core/asmcode.c
        CPPSOURCES += ../core/translate_arm.cpp
    else ifeq "$(TOOLCHAIN_ARCH)" "aarch64"
        ASMSOURCES += ../core/asmcode_aarch64.S
        CSOURCES += ../core/asmcode.c
        CPPSOURCES += ../core/translate_aarch64.cpp
    else
        $(error Unknown architecture $(TOOLCHAIN_ARCH))
    endif
else
    CSOURCES += ../core/asmcode.c
    FLAGS += -DNO_TRANSLATION
endif

ifeq "$(SUPPORT_LINUX)" "TRUE"
    FLAGS += -DSUPPORT_LINUX
endif

CFLAGS += -std=c11 $(FLAGS)
CXXFLAGS += -std=c++11 $(FLAGS)
LFLAGS +=
LIBS := -lz

CSOURCES   += ../core/armsnippets_loader.c ../core/casplus.c ../core/des.c ../core/disasm.c ../core/gdbstub.c \
              ../core/interrupt.c ../core/lcd.c ../core/link.c ../core/mem.c ../core/misc.c \
              ../core/mmu.c ../core/schedule.c ../core/serial.c ../core/sha256.c ../core/usb.c ../core/usblink.c \
              ../core/os/os-linux.c

CPPSOURCES += ../core/arm_interpreter.cpp ../core/coproc.cpp ../core/cpu.cpp ../core/debug.cpp ../core/emu.cpp \
              ../core/flash.cpp ../core/gif.cpp ../core/thumb_interpreter.cpp ../core/usblink_queue.cpp main.cpp \
              ../core/keypad.cpp ../core/cx2.cpp ../core/usb_cx2.cpp

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
	$(CXX) $(CXXFLAGS) $(LFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(OBJS) $(OUTPUT)
