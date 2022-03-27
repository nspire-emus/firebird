lessThan(QT_MAJOR_VERSION, 5): error("You need at least Qt 5.9 to build Firebird!")
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9): error("You need at least Qt 5.9 to build Firebird!")

# Version
DEFINES += FB_VERSION=1.5-dev

# JIT. Set to true, false or auto (default)
isEmpty(TRANSLATION_ENABLED): TRANSLATION_ENABLED = auto
# More accurate, but slower. Set to true, false or auto (default)
isEmpty(SUPPORT_LINUX) | equals(SUPPORT_LINUX, auto) {
	SUPPORT_LINUX = true
	ios|android: SUPPORT_LINUX = false
}

# Localization
TRANSLATIONS += i18n/de_DE.ts i18n/fr_FR.ts i18n/pl_PL.ts

QT += core gui widgets quickwidgets
android: QT += androidextras
CONFIG += c++11

TEMPLATE = app
TARGET = firebird-emu

# Warn if git submodules not downloaded
!exists("core/gif-h/gif.h"): error("You have to run 'git submodule init' and 'git submodule update' first.")

unix: !android {
    # For make install support
    isEmpty(PREFIX): PREFIX = /usr/local
    message("PREFIX: $$PREFIX")
    target.path = $$PREFIX/bin
    desktop.path = $$PREFIX/share/applications
    desktop.files += resources/org.firebird-emus.firebird-emu.desktop
    icon.path = $$PREFIX/share/icons/hicolor/512x512/apps
    icon.files += resources/org.firebird-emus.firebird-emu.png
    sendtool.path = $$PREFIX/bin
    sendtool.CONFIG = nostrip
    sendtool.files = core/firebird-send
    INSTALLS += target desktop icon sendtool
}

QMAKE_CFLAGS += -g -std=gnu11 -Wall -Wextra
QMAKE_CXXFLAGS += -g -Wall -Wextra -D QT_NO_CAST_FROM_ASCII
LIBS += -lz

# Override bad default options to enable better optimizations
QMAKE_CFLAGS_RELEASE = -O3 -DNDEBUG
QMAKE_CXXFLAGS_RELEASE = -O3 -DNDEBUG

# Don't enable LTO with clang on Linux, incompatible with Qt (QTBUG-43556).
# On FreeBSD, clang with LTO produces copy relocs, which are incompatible
# with Qt's -reduce-relocations option (QTBUG-86173).
!clang | !if(linux|freebsd): CONFIG += ltcg

# noexecstack is not supported by MinGW's as
!win32 {
    QMAKE_CFLAGS += -Wa,--noexecstack
}

# Work around https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
win32: {
    QMAKE_CFLAGS += -mno-ms-bitfields
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

macx: ICON = resources/logo.icns

# This does also apply to android
unix: SOURCES += core/os/os-linux.c

android {
    # Special implementation of fopen_utf8
    SOURCES += core/os/os-android.cpp
    # Fix up all relocations for known symbols, makes it faster and gets rid of some .text relocations
    # if disabled in asmcode_arm.S.
    QMAKE_LFLAGS += -Wl,-Bsymbolic
}

ios|android: DEFINES += MOBILE_UI

ios {
    DEFINES += IS_IOS_BUILD
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_CFLAGS += -mno-thumb
    QMAKE_CXXFLAGS += -mno-thumb
    QMAKE_LFLAGS += -mno-thumb
    ios_icon.files = $$files(resources/ios/Icon*.png)
    QMAKE_BUNDLE_DATA += ios_icon
}

# QMAKE_HOST can be e.g. armv7hl, but QT_ARCH would be arm in such cases
FB_ARCH = $$QT_ARCH

# arm64, arm64-v8a
contains(FB_ARCH, "arm64.*") {
    FB_ARCH = aarch64
}

# armv7l, armeabi-v7a
contains(FB_ARCH, "arm.*") {
    FB_ARCH = arm
}

equals(FB_ARCH, "i386") {
    FB_ARCH = x86
}

win32 {
    SOURCES += core/os/os-win32.c
    LIBS += -lwinmm -lws2_32
}

# A platform-independant implementation of lowlevel access as default
ASMCODE_IMPL = core/asmcode.c

TRANSLATE = $$files("core/translate_"$$FB_ARCH".c*")
equals(TRANSLATION_ENABLED, auto) {
    isEmpty(TRANSLATE): TRANSLATION_ENABLED = false
    else: TRANSLATION_ENABLED = true
}

equals(TRANSLATION_ENABLED, true) {
    isEmpty(TRANSLATE): error("No JIT found for arch $$FB_ARCH")
    SOURCES += $$TRANSLATE

    ASMCODE_IMPL = $$files("core/asmcode_"$$FB_ARCH".S")
    # Only the asmcode_x86.S functions can be called from outside the JIT
    !equals(FB_ARCH, "x86") {
        ASMCODE_IMPL += core/asmcode.c
    }
}
else: DEFINES += NO_TRANSLATION

message("FB_ARCH: $$FB_ARCH")
message("TRANSLATION_ENABLED: $$TRANSLATION_ENABLED")
message("SUPPORT_LINUX: $$SUPPORT_LINUX")

equals(SUPPORT_LINUX, true) {
    DEFINES += SUPPORT_LINUX
}

# Default to armv7 on ARM for movw/movt. If your CPU doesn't support it, comment this out.
!android:contains(FB_ARCH, "arm") {
    QMAKE_CFLAGS += -march=armv7-a -marm
    QMAKE_CXXFLAGS += -march=armv7-a -marm
}

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

QML_IMPORT_PATH += $$PWD/qml

SOURCES += $$ASMCODE_IMPL \
    core/fieldparser.cpp \
    lcdwidget.cpp \
    mainwindow.cpp \
    main.cpp \
    emuthread.cpp \
    qmlbridge.cpp \
    qtkeypadbridge.cpp \
    core/arm_interpreter.cpp \
    core/coproc.cpp \
    core/cpu.cpp \
    core/thumb_interpreter.cpp \
    core/usblink_queue.cpp \
    core/armsnippets_loader.c \
    core/casplus.c \
    core/des.c \
    core/disasm.c \
    core/gdbstub.c \
    core/gif.cpp \
    core/interrupt.c \
    core/keypad.cpp \
    core/lcd.c \
    core/link.c \
    core/mem.c \
    core/misc.c \
    core/mmu.c \
    core/schedule.c \
    core/serial.c \
    core/sha256.c \
    core/usb.c \
    core/usb_cx2.cpp \
    core/usblink.c \
    core/usblink_cx2.cpp \
    qtframebuffer.cpp \
    core/debug.cpp \
    core/flash.cpp \
    core/emu.cpp \
    usblinktreewidget.cpp \
    kitmodel.cpp \
    fbaboutdialog.cpp \
    dockwidget.cpp \
    consolelineedit.cpp \
    core/cx2.cpp

FORMS += \
    mainwindow.ui

HEADERS += \
    core/fieldparser.h \
    core/usb_cx2.h \
    core/usblink_cx2.h \
    emuthread.h \
    lcdwidget.h \
    mainwindow.h \
    keymap.h \
    qmlbridge.h \
    qtkeypadbridge.h \
    core/os/os.h \
    core/armcode_bin.h \
    core/armsnippets.h \
    core/asmcode.h \
    core/bitfield.h \
    core/casplus.h \
    core/cpu.h \
    core/cpudefs.h \
    core/debug.h \
    core/des.h \
    core/disasm.h \
    core/emu.h \
    core/flash.h \
    core/gdbstub.h \
    core/gif.h \
    core/interrupt.h \
    core/keypad.h \
    core/lcd.h \
    core/link.h \
    core/mem.h \
    core/misc.h \
    core/mmu.h \
    core/schedule.h \
    core/sha256.h \
    core/translate.h \
    core/usb.h \
    core/usblink.h \
    core/usblink_queue.h \
    qtframebuffer.h \
    usblinktreewidget.h \
    kitmodel.h \
    fbaboutdialog.h \
    dockwidget.h \
    consolelineedit.h \
    core/cx2.h

# For localization
lupdate_only {
SOURCES += qml/Keypad.qml \
    qml/MobileUI.qml \
    qml/ConfigPagesModel.qml \
    qml/NNumButton.qml \
    qml/MobileControl2.qml \
    qml/SidebarButton.qml \
    qml/ConfigPageDebug.qml \
    qml/ConfigPageEmulation.qml \
    qml/ConfigPageFileTransfer.qml \
    qml/ConfigPageKits.qml \
    qml/NBigButton.qml \
    qml/NButton.qml \
    qml/NAlphaButton.qml \
    qml/NDualButton.qml \
    qml/FBConfigDialog.qml \
    qml/FlashDialog.qml \
    qml/Touchpad.qml \
    qml/Firebird/Emu/Emu.qml \
    qml/Firebird/Emu/EmuScreen.qml \
    qml/Firebird/UIComponents/FBLabel.qml \
    qml/Firebird/UIComponents/FileSelect.qml \
    qml/Firebird/UIComponents/PageDelegate.qml \
    qml/Firebird/UIComponents/PageList.qml \
    qml/Firebird/UIComponents/ConfigPages.qml
}

# This doesn't exist, but Qt Creator ignores that
just_show_up_in_qt_creator {
SOURCES += core/asmcode_arm.S \
    core/asmcode_aarch64.S \
    core/asmcode_x86.S \
    core/asmcode_x86_64.S \
    core/asmcode.c \
    core/os/os-emscripten.c \
    core/translate_arm.cpp \
    core/translate_aarch64.cpp \
    core/translate_x86.c \
    core/translate_x86_64.c \
    headless/main.cpp \
    emscripten/main.cpp
}

RESOURCES += \
    resources.qrc

DISTFILES += \
    resources/org.firebird.firebird-emu.desktop \
    core/firebird-send \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle

# Generate the binary arm code into armcode_bin.h
armsnippets.commands = arm-none-eabi-gcc -fno-leading-underscore -c $$PWD/core/armsnippets.S -o armsnippets.o -mcpu=arm926ej-s \
                        && arm-none-eabi-objcopy -O binary armsnippets.o snippets.bin \
                        && xxd -i snippets.bin > $$PWD/core/armcode_bin.h \
                        && rm armsnippets.o

# In case you change armsnippets.S, run "make armsnippets" and update armcode_bin.h
QMAKE_EXTRA_TARGETS = armsnippets
