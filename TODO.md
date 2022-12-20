##CX II:
* Standby needs fast timer + GPIO bits
* Check other aladdin PMU bits
* Flash dialog needs to be adapted (rewrite in QML anyway?)
* Refactor flash code, split controllers and image manipulation?
* Handle the LCD and "Magic VRAM" correctly
* Support "deep sleep"
* Inform when an incomplete boot rom is used

##TODO:
* Implement write_action for non-x86 to handle SMC and clearing RF_CODE_NO_TRANSLATE correctly
* File transfer: Move by D'n'D, drop folders, download folders
* Better debugger integration
* Don't use a 60Hz timer for LCD redrawing, hook lcd_event instead
* Less global vars (emu.h), move into structs
* Use streams for reading/writing snapshots instead of struct emu_snapshot
* Fastboot data is currently not cleared at all, it survives soft and hard
  resets as well as restarts. This was the simplest way to get installers to
  work, which require that state is persisted across software resets. Ideally,
  fastboot data is cleared on hardware resets and restarts.

##Wishlist:
* Skin loader/switcher
* Scripting support, somehow
* Expose the calc as a fake USB one
* Communication with USB peripherals over libusb
