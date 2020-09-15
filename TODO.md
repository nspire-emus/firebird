##CX II:
* Standby needs fast timer + GPIO bits
* Check other aladdin PMU bits
* usb doesn't work after resume if previously connected
* CX usblink doesn't work anymore
* Flash creation needs changes
* Load product from manuf
* Refactor flash code, split controllers and image manipulation?

##TODO:
* Implement write_action for non-x86 to handle SMC and clearing RF_CODE_NO_TRANSLATE correctly
* File transfer: Move by D'n'D, drop folders, download folders
* Better debugger integration
* Don't use a 60Hz timer for LCD redrawing, hook lcd_event instead
* Less global vars (emu.h), move into structs
* Support content:/ URLs returned by the native file dialog on Android with Qt 5.13
* Use streams for reading/writing snapshots instead of struct emu_snapshot 

##Wishlist:
* Skin loader/switcher
* Scripting support, somehow
* Expose the calc as a fake USB one
* Communication with USB peripherals over libusb
