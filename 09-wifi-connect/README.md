/opt/homebrew/bin/pio run -t upload -t monitor -e esp32dev
Processing esp32dev (platform: https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip; board: esp32dev; framework: arduino)
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32dev.html
PLATFORM: Espressif 32 (55.3.312) > Espressif ESP32 Dev Module
HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash
DEBUG: Current (cmsis-dap) External (cmsis-dap, esp-bridge, esp-prog, esp-prog-2, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
PACKAGES:
 - contrib-piohome @ 3.4.4
 - framework-arduinoespressif32 @ 3.3.12
 - framework-arduinoespressif32-libs @ 5.5.5+sha.b774170ff46
 - tool-esptoolpy @ 5.4.0
 - tool-xtensa-esp-elf-gdb @ 17.1.0+20260402
 - toolchain-xtensa-esp-elf @ 14.2.0+20260121
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 47 compatible libraries
Scanning dependencies...
Dependency Graph
|-- SignalQuality
|-- WifiKeeper
|-- WiFi @ 3.3.12
Building in release mode
Compiling .pio/build/esp32dev/src/main.cpp.o
Linking .pio/build/esp32dev/firmware.elf
Memory Type Usage Summary
┏━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━┓
┃ Memory Type/Section ┃ Used [bytes] ┃ Used [%] ┃ Remain [bytes] ┃ Total [bytes] ┃
┡━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━┩
│ Flash Code          │       666732 │          │                │               │
│    .text            │       666732 │          │                │               │
│ Flash Data          │       128044 │          │                │               │
│    .rodata          │       119664 │          │                │               │
│    .eh_frame        │         8124 │          │                │               │
│    .appdesc         │          256 │          │                │               │
│ IRAM                │        83915 │    64.02 │          47157 │        131072 │
│    .text            │        82887 │    63.24 │                │               │
│    .vectors         │         1028 │     0.78 │                │               │
│ DRAM                │        45845 │     36.8 │          78735 │        124580 │
│    .data            │        23933 │    19.21 │                │               │
│    .bss             │        21912 │    17.59 │                │               │
└─────────────────────┴──────────────┴──────────┴────────────────┴───────────────┘
Total image size: 902624 bytes (.bin may be padded larger)
Note: The reported total sizes may be smaller than those in the technical reference manual due to reserved memory and application configuration. The total flash size available for the application is not included by default, as it cannot be reliably determined due to the presence of other data like the bootloader, partition table, and application partition size.
Retrieving maximum program size .pio/build/esp32dev/firmware.elf
Checking size .pio/build/esp32dev/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  14.0% (used 45848 bytes from 327680 bytes)
Flash: [=======   ]  68.2% (used 894244 bytes from 1310720 bytes)
Building .pio/build/esp32dev/firmware.bin
esptool v5.4.0
Creating ESP32 image...
Merged 3 ELF sections.
Successfully created ESP32 image.
Creating binary "firmware.factory.bin" with:
    Offset   | File
 -  0x1000   | bootloader.bin
 -  0x8000   | partitions.bin
 -  0xe000   | boot_app0.bin
 -  0x10000  | firmware.bin
Successfully created combined binary image.
Configuring upload protocol...
AVAILABLE: cmsis-dap, esp-bridge, esp-prog, esp-prog-2, espota, esptool, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa
CURRENT: upload_protocol = esptool
Looking for upload port...
Auto-detected: /dev/cu.usbserial-210
Uploading .pio/build/esp32dev/firmware.bin
esptool v5.4.0
Serial port /dev/cu.usbserial-210:
Connecting.....
Connected to ESP32 on /dev/cu.usbserial-210:
Chip type:          ESP32-D0WD-V3 (revision v3.1)
Features:           Wi-Fi, BT, Dual Core + LP Core, 240MHz, Vref calibration in eFuse, Coding Scheme None
Crystal frequency:  40MHz
MAC:                20:9b:a9:61:15:6c

Uploading stub flasher...
Running stub flasher...
Stub flasher running.
Changing baud rate to 460800...
Changed.

Configuring flash size...
Auto-detected flash size: 4MB

Writing '/Users/pavlopobereznij/Documents/projects/learn/embeded/esp32-lab/09-wifi-connect/.pio/build/esp32dev/bootloader.bin' at 0x00001000...
SHA digest in image updated.
Flash will be erased from 0x00001000 to 0x00006fff...
Compressed 23520 bytes to 15099...

Writing at 0x00006be0 [██████████████████████████████] 100.0% 14.75kB/14.75kB [0s]
Wrote 23520 bytes (15099 compressed) at 0x00001000 in 0.8 seconds (242.1 kbit/s).
Verifying written data...
Hash of data verified.

Writing '/Users/pavlopobereznij/Documents/projects/learn/embeded/esp32-lab/09-wifi-connect/.pio/build/esp32dev/partitions.bin' at 0x00008000...
Flash will be erased from 0x00008000 to 0x00008fff...
Compressed 3072 bytes to 146...

Writing at 0x00008c00 [██████████████████████████████] 100.0% 146B/146B [0s]
Wrote 3072 bytes (146 compressed) at 0x00008000 in 0.1 seconds (361.3 kbit/s).
Verifying written data...
Hash of data verified.

Writing '/Users/pavlopobereznij/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin' at 0x0000e000...
Flash will be erased from 0x0000e000 to 0x0000ffff...
Compressed 8192 bytes to 47...

Writing at 0x00010000 [██████████████████████████████] 100.0% 47B/47B [0s]
Wrote 8192 bytes (47 compressed) at 0x0000e000 in 0.1 seconds (474.8 kbit/s).
Verifying written data...
Hash of data verified.

Writing '.pio/build/esp32dev/firmware.bin' at 0x00010000...
Flash will be erased from 0x00010000 to 0x000ecfff...
Compressed 902768 bytes to 587125...

Writing at 0x000ec670 [██████████████████████████████] 100.0% 573.36kB/573.36kB [15s]
Wrote 902768 bytes (587125 compressed) at 0x00010000 in 16.0 seconds (452.1 kbit/s).
Verifying written data...
Hash of data verified.

Hard resetting via RTS pin...
--- Terminal on /dev/cu.usbserial-210 | 115200 8-N-1
--- Available filters and text transformations: debug, default, direct, esp32_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H

09-wifi-connect
SSID: free-wifiц, timeout 15000 ms, backoff 1000..30000 ms
Scanning...
Scan: 15 networks
   1) ASUS_4C                          ch=13 rssi= -59 (good) secured
   2) free-wifi                        ch= 2 rssi= -65 (fair) secured
   3) TP-Link_IoT_CE36                 ch= 2 rssi= -65 (fair) secured
   4) TP-Link_906D                     ch= 9 rssi= -67 (fair) secured
   5) TP-LINK_1C2C                     ch= 6 rssi= -79 (weak) secured
   6) kissska                          ch= 9 rssi= -83 (unusable) secured
   7) Oleftina_2G                      ch= 7 rssi= -89 (unusable) secured
   8) TP-Link_14AE                     ch= 1 rssi= -90 (unusable) secured
   9) 112                              ch= 2 rssi= -91 (unusable) secured
  10) Repeater_46E56C                  ch= 6 rssi= -92 (unusable) secured
  11) Tenda_7CE1D0                     ch= 6 rssi= -93 (unusable) secured
  12) Denis                            ch= 1 rssi= -94 (unusable) secured
  13) 90                               ch= 1 rssi= -94 (unusable) secured
  14) Xiaomi_282D                      ch= 5 rssi= -94 (unusable) secured
  15) TP-LINK_3674                     ch= 6 rssi= -96 (unusable) secured
Ready
Connecting to free-wifiц (attempt 1)...
Status: not connected (attempt 1)
Disconnect reason 36: STA_LEAVING (total 2)
Disconnect reason 201: NO_AP_FOUND (total 3)
Status: not connected (attempt 1)
Status: not connected (attempt 1)
Connect timeout after 15000 ms, retry in 1000 ms
Status: not connected (attempt 1)
Connecting to free-wifiц (attempt 2)...
Disconnect reason 36: STA_LEAVING (total 4)
Disconnect reason 201: NO_AP_FOUND (total 5)
Status: not connected (attempt 2)


