nspire_emu
==========

This project is currently the main TI-Nspire/CAS/Touchpad emulator.  
The original piece of software can be found in the "mirror" branch or in the original repository located at ~~https://www.unsads.com/scm/svn/nsptools/Ndless/trunk/Ndless-SDK/nspire_emu/~~. The new repo location is in 
Github https://github.com/OlivierA/Ndless.

Either new instructions about how to mirror will be made or a new common repo will be made after talking to them.

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

