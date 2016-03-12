Firebird Emu [![Build Status](https://travis-ci.org/nspire-emus/firebird.svg)](https://travis-ci.org/nspire-emus/firebird) <img align="right" src="https://i.imgur.com/TE0Bxm8.png">
==========

This project is currently the community TI-Nspire emulator, originally created by Goplat.  
It supports the emulation of Touchpad, CX and CX CAS calcs on Android, iOS, Linux, Mac and Windows.

Download:
---------

* *Latest release* (Windows, Mac, Android, iOS): https://github.com/nspire-emus/firebird/releases/latest
* *Repo for Arch and openSUSE*: https://software.opensuse.org/download.html?project=home%3AVogtinator%3Afirebird-emu&package=firebird-emu

Screenshots/Videos:
-------------------

_Linux:_

[![](http://i.imgur.com/eGJOMsSl.png)](http://i.imgur.com/eGJOMsS.png)

_Windows:_ | _Android:_
--- | ---
[![](https://i.imgur.com/aibTt9Cl.png)](https://i.imgur.com/aibTt9C.png) | [![](http://i.imgur.com/fpN81kom.png)](http://i.imgur.com/fpN81ko.png)
_Mac OS X:_ | _iOS:_
[![](https://i.imgur.com/5Rl7VTG.png)](https://i.imgur.com/YhqkwIN.png) | [![](https://i.imgur.com/Gly3Mzhm.png)](https://i.imgur.com/Gly3Mzh.png)

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
There seems to be a bug in qmake that makes the required .S get ignored when building a JIT-enabled ([`TRANSLATION_ENABLED = true`](https://github.com/nspire-emus/firebird/blob/master/firebird.pro#L4)) binary.  
As a workaround, you'll have to take care of it manually:

1. start a standard build so that all the needed files get created
2. in Terminal, `cd` to the `src/core` directory
3. get the .o yourself: `clang -arch armv7 -marm -o asmcode_arm.o -c asmcode_arm.S`
4. move this newly created `asmcode_arm.o` into the objects directory (`[BUILD_FOLDER]/firebird.build/Release-iphoneos/firebird.build/Objects-normal/armv7/`)
5. add a line with `asmcode_arm.o` to the `firebird.LinkFileList` file (which is also located in the objects folder)
6. lock both files (Right-Click -> Get Info)

Now that these files are locked, the next build won't be able to overwrite them, and it should be fine to rebuild.  
You should then be able to Deploy to your iOS device. Don't forget to transfer the boot1 and flash, from within iTunes, for instance.


License
-------
This work (except the icons from the KDE project) is licensed under the GPLv3.
