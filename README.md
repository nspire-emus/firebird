nspire_emu
==========

This project is currently the main TI-Nspire/CAS/Touchpad emulator started by Goplat.  
The original piece of software can be found in the "mirror" branch or in the original repository located inside the https://github.com/OlivierA/Ndless repository.


Building
--------

In windows mingw is needed along with the arm-none-eabi compiler suite:
```bash
cd src
make
```

If someone wants to compile the windows executable in linux the mingw compiler suite is needed:
```bash
cd src
OPTS=mingw make
```
For executing the previous wine is needed.

Linux native binaries cannot be compiled yet but a simple ```make``` should do it in the future.


Coding conventions
------------------

1. Line width: I don't believe in the 80 character limit as modern computers are capable of displaying that and more. Anyway if anyone feels that a line is way to long may want to split it.
2. File headers: Each file should contain a header explaining the purpose of the code contained in it.
3. Includes: Source includes should be before system includes. This will avoid duplicated includes and will ensure that when a header file is included in a new file less errors will be generated.
