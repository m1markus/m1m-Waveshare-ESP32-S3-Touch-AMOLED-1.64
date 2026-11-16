# Image assets

Images were converted to the LVGL binary format by LCD Wizard - there is
no separate conversion step. Everything in this folder goes onto the board.

## Files for the SPIFFS filesystem

| File | Source | Size | Format | Referenced as |
|---|---|---|---|---|
| `bitcoin.bin` | bitcoin.svg | 64x64 | RGB565A8 | `S:/bitcoin.bin` |
| `bitcoin_2.bin` | bitcoin.png | 128x128 | RGB565A8 | `S:/bitcoin_2.bin` |

Total: **60.0 KB** of roughly 3.88 MB available.

## Uploading

The files must sit in a folder named `data/` next to the `.ino`, then be
flashed as a filesystem image - they are **not** part of the sketch upload.

### PlatformIO

```bash
pio run -t uploadfs
```

### Arduino IDE

**The Arduino IDE 2 SPIFFS uploader cannot be used with this project.** It takes the filesystem's
position from Tools > Partition Scheme and knows only the core's stock schemes, and none of them
is this N16R8 layout. Two ways round it:

- **Switch to LittleFS** in LCD Wizard (Assets > Filesystem) and export again. Its uploader,
  arduino-littlefs-upload, reads `partitions.csv` from the sketch folder and works with any layout.
- Or build and write the image yourself, with the two tools the ESP32 core installed (under
  `Arduino15/packages/esp32/tools/`, in `mkspiffs` and `esptool_py`). From the sketch folder:

```bash
mkspiffs -c data -p 256 -b 4096 -s 0x3E0000 spiffs.bin
esptool --chip esp32s3 --port <PORT> --baud 921600 write_flash -z 0xC10000 spiffs.bin
```

The filesystem partition is set by `partitions.csv` in the project folder -
choose a layout with more room for files on the Hardware tab if these do not fit.

## How it is wired up

`ui_fs.cpp` registers SPIFFS with LVGL as drive `S:`, so a widget
refers to an image by path:

```c
lv_image_set_src(ui_Image1, "S:/bitcoin.bin");
```

`ui_fs_init()` runs from `setup()` before the UI is built, and prints a warning
for any file it cannot find so a missing image is obvious on the serial monitor.

## Changing an image

Re-import it in LCD Wizard and export again. Editing these files by hand means
the header and the pixel data can disagree, which shows up as a garbled image.
