lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5 to build nspire_emu!")

TRANSLATION_ENABLED = true

QT += core gui widgets
CONFIG += c++11

TEMPLATE = app
TARGET = nspire_emu

# For make install support
target.path = /usr/local/bin
INSTALLS += target

QMAKE_CFLAGS = -g -std=gnu11 -Wall -Wextra
QMAKE_CXXFLAGS = -g -std=c++11 -Wall -Wextra

# Override bad default options to enable better optimizations
QMAKE_CFLAGS_RELEASE = -O3 -flto -fwhole-program
QMAKE_CXXFLAGS_RELEASE = -O3 -flto -fwhole-program
QMAKE_LFLAGS_RELEASE = -Wl,-O3 -flto -fwhole-program

# ICE on mac with clang
macx-clang|ios {
    QMAKE_CFLAGS_RELEASE -= -flto -fwhole-program
    QMAKE_CXXFLAGS_RELEASE -= -flto -fwhole-program
    QMAKE_LFLAGS_RELEASE -= -Wl,-O3 -flto -fwhole-program
    ICON = resources/logo.icns
}

# This does also apply to android
linux|macx|ios {
    SOURCES += os/os-linux.c
}

ios {
    DEFINES += IS_IOS_BUILD __arm__
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_CFLAGS += -mno-thumb
    QMAKE_CXXFLAGS += -mno-thumb
    QMAKE_LFLAGS += -mno-thumb
    QMAKE_IOS_DEVICE_ARCHS = armv7
}

# QMAKE_HOST can be e.g. armv7hl, but QT_ARCH would be arm in such cases
QMAKE_TARGET.arch = $$QT_ARCH

win32 {
    SOURCES += os/os-win32.c
    LIBS += -lwinmm -lws2_32
    # Somehow it's set to x86_64...
    QMAKE_TARGET.arch = x86
}

linux-g++-32 {
    QMAKE_CFLAGS += -m32
    QMAKE_CXXFLAGS += -m32
    QMAKE_TARGET.arch = x86
}

# A platform-independant implementation of lowlevel access as default
ASMCODE_IMPL = asmcode.c

equals(TRANSLATION_ENABLED, true) {
    TRANSLATE = $$join(QMAKE_TARGET.arch, "", "translate_", ".c")
    exists($$TRANSLATE) {
        SOURCES += $$TRANSLATE
    }

    TRANSLATE2 = $$join(QMAKE_TARGET.arch, "", "translate_", ".cpp")
    exists($$TRANSLATE2) {
        SOURCES += $$TRANSLATE2
    }

    ASMCODE = $$join(QMAKE_TARGET.arch, "", "asmcode_", ".S")
    exists($$ASMCODE): ASMCODE_IMPL = $$ASMCODE
}
else: DEFINES += NO_TRANSLATION

# The x86_64 and ARM JIT use asmcode.c for mem access
contains(QMAKE_TARGET.arch, "x86_64") || contains(QMAKE_TARGET.arch, "arm") {
    !contains(ASMCODE_IMPL, "asmcode.c") {
        SOURCES += asmcode.c
    }
}

# Default to armv7 on ARM for movw/movt If your CPU doesn't support it, comment this out.
contains(QMAKE_TARGET.arch, "arm") {
    QMAKE_CFLAGS += -march=armv7-a -marm
    QMAKE_CXXFLAGS += -march=armv7-a -marm
    QMAKE_LFLAGS += -march=armv7-a -marm # We're using LTO, so the linker has to get the same flags
}

SOURCES += $$ASMCODE_IMPL \
    lcdwidget.cpp \
    usblink_queue.cpp \
    cpu.cpp \
    mainwindow.cpp \
    main.cpp \
    armloader.c \
    casplus.c \
    debug.c \
    des.c \
    disasm.c \
    emu.c \
    flash.c \
    flashdialog.cpp \
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
    usb.c \
    usblink.c \
    emuthread.cpp \
    arm_interpreter.cpp \
    coproc.cpp \
    thumb_interpreter.cpp

FORMS += \
    mainwindow.ui \
    flashdialog.ui

HEADERS += \
    keypad.h \
    emu.h \
    emuthread.h \
    lcdwidget.h \
    usb.h \
    lcd.h \
    disasm.h \
    flash.h \
    flashdialog.h \
    interrupt.h \
    armcode_bin.h \
    mem.h \
    mmu.h \
    des.h \
    armsnippets.h \
    debug.h \
    sha256.h \
    usblink.h \
    mainwindow.h \
    keymap.h \
    misc.h \
    os/os.h \
    gdbstub.h \
    translate.h \
    cpu.h \
    casplus.h \
    link.h \
    asmcode.h \
    schedule.h \
    usblink_queue.h \
    cpudefs.h \
    bitfield.h

# Generate the binary arm code into armcode_bin.h
armsnippets.commands = arm-none-eabi-gcc -fno-leading-underscore -c $$PWD/armsnippets.S -o armsnippets.o -mcpu=arm926ej-s \
						&& arm-none-eabi-objcopy -O binary armsnippets.o snippets.bin \
						&& xxd -i snippets.bin > $$PWD/armcode_bin.h \
						&& rm armsnippets.o

# In case you change armsnippets.S, run "make armsnippets" and update armcode_bin.h
QMAKE_EXTRA_TARGETS = armsnippets

OTHER_FILES += \
    TODO

RESOURCES += \
    resources.qrc
