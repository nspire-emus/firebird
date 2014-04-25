nspire_emu
==========

This project is currently the main TI-Nspire/CAS/Touchpad emulator.  
The original piece of software can be found in the "mirror" branch or in the original repository located at https://www.unsads.com/scm/svn/nsptools/Ndless/trunk/Ndless-SDK/nspire_emu/.

How do I update the mirror
--------------------------
```
git checkout mirror
svn co https://www.unsads.com/scm/svn/nsptools/Ndless/trunk/Ndless-SDK/nspire_emu/ --username guest --password guest .
rm -r .svn
git add *
git commit -am "Updated with master."
git push
git checkout master
```
