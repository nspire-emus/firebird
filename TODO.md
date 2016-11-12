##TODO:
* Optimize ARM JIT: Implement literal pool
* File transfer: Move by D'n'D, drop folders, download folders
* Better debugger integration
* Don't use a 60Hz timer for LCD redrawing, hook lcd_event instead
* Implement DockWidget locking for better space usage: https://quickgit.kde.org/?p=dolphin.git&a=blob&f=src%2Fdolphindockwidget.cpp
* Less global vars (emu.h), move into structs

##Wishlist:
* Language selection at runtime?
* Skin loader/switcher
* Embedded updater (checker/downloader/installer)
* Scripting support, somehow
* Expose the calc as a fake USB one
* Communication with USB peripherals over libusb
