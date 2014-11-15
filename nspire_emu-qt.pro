QT += core gui widgets
CONFIG += c++11

TEMPLATE = app
TARGET = nspire_emu

QMAKE_CFLAGS = -O3 -std=gnu11 -Wall -Wextra -flto

#Override bad default options
QMAKE_CFLAGS_RELEASE = -O3
QMAKE_CXXFLAGS_RELEASE = -O3

#This does also apply to android
linux|macx {
	SOURCES += os/os-linux.c
}

win32 {
	SOURCES += os/os-win32.c
	LIBS += -lwinmm -lws32_2
}

#A platform-independant implementation of lowlevel access
ASMCODE_IMPL = asmcode.c

win32|linux-g++-32 {
	ASMCODE_IMPL = asmcode_x86.S
}

SOURCES += $$ASMCODE_IMPL
SOURCES += mainwindow.cpp \
        main.cpp \
        armloader.c \
        casplus.c \
        cpu.c \
        debug.c \
        des.c \
        disasm.c \
        emu.c \
        flash.c \
        gdbstub.c \
        interrupt.c \
        keypad.c \
        lcd.c \
        link.c \
        mem.c \
        misc.c \
        mmu.c \
        schedule.c \
        serial.c \
        sha256.c \
        translate.c \
        usb.c \
        usblink.c \
	emuthread.cpp

FORMS += \
    mainwindow.ui

HEADERS += \
    mainwindow.h \
	emuthread.h

#Generate the binary arm code into armcode_bin.h
armsnippets.commands = arm-none-eabi-gcc -fno-leading-underscore -c $$PWD/armsnippets.S -o armsnippets.o -mcpu=arm926ej-s \
						&& arm-none-eabi-objcopy -O binary armsnippets.o snippets.bin \
						&& xxd -i snippets.bin > $$PWD/armcode_bin.h \
						&& rm armsnippets.o

QMAKE_EXTRA_TARGETS = armsnippets
