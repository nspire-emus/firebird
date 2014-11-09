QT += core gui widgets
CONFIG += c++11

LIBS += -lreadline
QMAKE_CFLAGS += -DUSE_READLINE -O3

TEMPLATE = app
TARGET = nspire_emu

linux|macx {
	SOURCES += os/os-linux.c
}

win32 {
	SOURCES += os/os-windows.c
}

#Dirty hack to compile arm snippets without qmake interfering
QMAKE_PRE_LINK += arm-none-eabi-gcc -fno-leading-underscore -c $$PWD/armsnippets.S -o armsnippets.o -mcpu=arm926ej-s \
				&& arm-none-eabi-objcopy -O binary armsnippets.o snippets.bin \
				&& ld -melf_i386 -r -b binary -o armsnippets.o snippets.bin \
				&& rm snippets.bin \
				&& objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents armsnippets.o

QMAKE_LFLAGS += armsnippets.o

SOURCES += mainwindow.cpp \
		main.cpp \
		asmcode.S \
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
        memory.c \
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
