# Libraries for MyUI

Everything the generated sketch needs, with the exact names to search for.
Generated 2026-11-08 16:39.

## Install these

| Library | Author | Search for this in Arduino Library Manager | Version | Needed by |
|---|---|---|---|---|
| [lvgl](https://github.com/lvgl/lvgl) | LVGL Kft. | `lvgl` | **9.4.0** | The GUI library the whole UI is built on |
| [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) | Moon On Our Nation | `GFX Library for Arduino` | **1.6.9** | Drives the CO5300 panel |
| [SensorLib](https://github.com/lewisxhe/SensorLib) | Lewis He | `SensorLib` | **0.5.1** | "IMU" (QMI8658 (IMU)) |

Where a version is given, install that one: the export was built against it, and a
newer release is not known to compile.

> **lvgl** - Configured by the generated lv_conf.h, which the ESP32 core reads from the sketch folder - nothing to copy.

> **SensorLib** - One library covers both the QMI8658 IMU and the PCF85063 RTC.

## Already on your machine

Bundled with the board core - nothing to install.

- **Wire** - FT3168 / FT6146 is read directly by the generated sketch - no driver library needed
- **SD** - "SDCard" - bundled with the core
- **SPI** - SPI bus - bundled with the core

## Installing

### Arduino IDE

**Sketch > Include Library > Manage Libraries**, then search for each name in the
table above and click Install. Where the table gives a version, pick it from the list beside the Install button first.

### Arduino CLI

```bash
arduino-cli lib install "lvgl@9.4.0"
arduino-cli lib install "GFX Library for Arduino@1.6.9"
arduino-cli lib install "SensorLib@0.5.1"

arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=custom,CDCOnBoot=cdc .
```

### PlatformIO

Already written into `platformio.ini`, so `pio run` fetches them for you:

```ini
lib_deps =
    lvgl/lvgl@9.4.0
    moononournation/GFX Library for Arduino@1.6.9
    lewisxhe/SensorLib@0.5.1
```

## Versions this project was generated against

| | |
|---|---|
| LVGL | 9.4.0 (API 9.x) |
| Board core | ESP32-S3 - `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=custom,CDCOnBoot=cdc` |
| Graphics backend | GFX Library for Arduino |

LVGL 8 and 9 are not source compatible. If you switch major version, re-export
from LCD Wizard rather than hand-patching the generated files.
