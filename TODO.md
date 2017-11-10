##TODO:
* Implement write_action for non-x86 to handle SMC and clearing RF_CODE_NO_TRANSLATE correctly
* File transfer: Move by D'n'D, drop folders, download folders
* Better debugger integration
* Don't use a 60Hz timer for LCD redrawing, hook lcd_event instead
* Less global vars (emu.h), move into structs

##Wishlist:
* Language selection at runtime?
* Skin loader/switcher
* Scripting support, somehow
* Expose the calc as a fake USB one
* Communication with USB peripherals over libusb
