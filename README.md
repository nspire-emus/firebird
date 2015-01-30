nspire_emu
==========

This project is currently the community TI-Nspire emulator, originally created by Goplat.

Supported models : Prototypes CAS+, Clickpad/Touchpad/CX (non-CAS and CAS), LabStation.

 

*The most updated branch (and the only one under dev.) is the Qt one.*

Building
--------

First, you need to install Qt5.

Then, you can either use Qt Creator directly (don't forget to configure your kits/compilers etc. !), or run :

```
mkdir -p build
cd build
qmake ..
make
```

Coding conventions
------------------

1. Line width: I don't believe in the 80 character limit as modern computers are capable of displaying that and more. Anyway if anyone feels that a line is way to long may want to split it.
2. Indents: by 4 spaces.
3. File headers: Each file should contain a header explaining the purpose of the code contained in it.
4. Includes: Source includes should be before system includes. This will avoid duplicated includes and will ensure that when a header file is included in a new file less errors will be generated.
