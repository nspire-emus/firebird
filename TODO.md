##TODO:
* Optimize x86_64 JIT: {read,write}_{byte,half,word} in asm
* File transfer: Move by D'n'D, drop folders, download folders
* Better debugger integration
* Don't use a 60Hz timer for LCD redrawing, hook lcd_event instead
* Implement QLabel with elide, like https://forum.qt.io/topic/24530/solved-shortening-a-label
* Implement DockWidget locking for better space usage: https://quickgit.kde.org/?p=dolphin.git&a=blob&f=src%2Fdolphindockwidget.cpp

##Wishlist:
* Language selection at runtime?
* Skin loader/switcher
* Embedded updater (checker/downloader/installer)
* Scripting support, somehow
* Expose the calc as a fake USB one
* Communication with USB peripherals over libusb
* Different "kits": Selectable sets of flash/boot1/settings
* Get the touchpad working in a way the OS's cursor likes
