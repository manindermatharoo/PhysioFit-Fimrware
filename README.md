# physio_tracker_fw
Firmware Dev for FYDP

Need to change the linker file for nrf board located at__
"Users/name/Library/Arduino15/packages/adafruit/hardware/nrf52/0.16.0/cores/nRF5/linker/nrf52832_s132_v6.ld"__
Change to RAM (rwx) :  ORIGIN = 0x20004598, LENGTH = 0x20010000 - 0x20004598__
This is needed as having more than 1 BLE Connection needs more SRAM__

In order to use the libraries located at Fimrware/libraries they need to be added to Arduino__
