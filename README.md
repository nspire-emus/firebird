Firebird Emu
==========

This project is currently the community TI-Nspire emulator, originally created by Goplat.  
It supports the emulation of Touchpad, CX and CX CAS calcs on Android, iOS, Linux, Mac and Windows.

Building
--------

First, you need to install Qt5.  
Then, you can either use Qt Creator directly (don't forget to configure your kits/compilers etc.!), or run:

```
mkdir -p build
cd build
qmake ..
make
```
#####A special case: iOS with translation (≈JIT) enabled:
There seems to be a bug in qmake that makes the required .S get ignored in the build process.  
As a workaround, you'll have to include it manually:

1. start a standard build so that all the needed files get created
2. `mv` to the `src/core` directory
3. get the .o yourself: `clang -marm -o asmcode_arm.o -c asmcode_arm.S`
4. move `asmcode_arm.o` in the objects folder
5. add `asmcode_arm.o` to the `.linklistfile`
6. lock both files (Right-Click -> Get Info) so that the next rebuild won't overwrite them

Now that these files are locked, the next build won't be able to overwrite them, and it should be fine.  
You should then be able to Deploy to your iOS device. Don't forget to transfer the boot1 and flash, from within iTunes, for instance.


License
-------
This work (except the icons from the KDE project) is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License](http://creativecommons.org/licenses/by-sa/4.0/)
