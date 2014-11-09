nspire_emu
==========

This project is currently the main TI-Nspire/CAS/Touchpad emulator started by Goplat.  


Building
--------

You need to install Qt5 with 32bit support.

Run
```
mkdir -p build
cd build
qmake ..
make
```
You have to use "qmake -spec linux-g++-32 .." if you're building on a 32bit system.

For mac, use "qmake -spec macx-g++32 ..".

TODO: Figure out whether that works and find a way to compile it for windows.


Coding conventions
------------------

1. Line width: I don't believe in the 80 character limit as modern computers are capable of displaying that and more. Anyway if anyone feels that a line is way to long may want to split it.
2. File headers: Each file should contain a header explaining the purpose of the code contained in it.
3. Includes: Source includes should be before system includes. This will avoid duplicated includes and will ensure that when a header file is included in a new file less errors will be generated.
