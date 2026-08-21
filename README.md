# ESP32 DeskDog ROM（for Micro TFT LCD)

ESP32 DeskDog ROM is a customized ESP32 firmware project based on the open-source [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) project.
It preserves the original AI voice interaction, TTS playback, and emotional expression features while adding **real-time AI response subtitles** to a 0.96-inch 160×80 TFT display.

The screen combines animated emotional expressions with scrolling text, allowing users to both **see and hear** the robot's responses and providing a more intuitive and engaging interaction experience.

## Features

* Designed for ESP32-based desktop robot dogs
* Real-time AI response subtitles
* Emotional expressions and animations
* Voice interaction and TTS playback
* Horizontal scrolling for longer responses
* Optimized display layout for a 160×80 TFT screen
* Pre-built firmware binaries included
* Source code and project configuration included

## Display

The 160×80 display is divided into two main areas:
* **Upper area:** Emotional animations such as happy, thinking, surprised, and sad
* **Bottom area:** Real-time AI response text with horizontal scrolling

The display layout is optimized for the very limited screen space, prioritizing both emotional expressions and readable subtitles.

## Credits
This project is based on:
* **Original Project:** [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)

The original project provides the underlying ESP32 AI voice interaction framework and related functionality.
This repository contains personal modifications and customizations for the ESP32 DeskDog.

## Repository Contents

The repository includes:
* Source code
* `managed_components/` — project dependencies
* `sdkconfig` — ESP-IDF project configuration
* `dependencies.lock` — dependency version information
* Pre-built `.bin` firmware files

## How to Build and Flash

This project can be built and flashed directly using **ESP-IDF**.
### 1. Clone the Repository
open powershell
```bash
git clone https://github.com/prodigylinda/ESP32_deskdog_ROM.git
cd ESP32_deskdog_ROM
```
### 2. Set Up ESP-IDF
Make sure **ESP-IDF** is installed and configured correctly.
Refer to the official [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) for installation instructions.
For this project, **ESP-IDF 6.0 or later** is recommended.

### 3. Configure the Project
Configure the project for your target ESP32 board:
```bash
idf.py set-target esp32
```
You can also modify the project configuration if necessary:
```bash
idf.py menuconfig
```
（top/xiaozhi_assistant/boards...)
If your ESP32 has a different flash size, make sure to change it in:

**`menuconfig → Serial flasher config → Flash size`**

After changing the flash size, save the configuration so that the corresponding settings are updated in the project's `sdkconfig.defaults` and `sdkconfig`file.
For example, this project was originally configured for a **4 MB flash**. Accidentally selecting **16 MB** caused flashing errors on a 4 MB device.

### 4. Build the Firmware
Build the project using:
```bash
idf.py build
```
The compiled firmware files will be generated in the `build/` directory.

### 5. Flash the Firmware
Connect your ESP32 device to your computer and flash the firmware with:
```bash
idf.py flash
```
If the device is connected through a specific serial port, you can specify it with:
```bash
idf.py -p COM3 flash
```
Replace `COM3` with the actual port of your ESP32 device.

### 6. Monitor the Device
After flashing, you can view the serial output with:
```bash
idf.py monitor
```
Or build, flash, and monitor in one command:
```bash
idf.py build flash monitor
```

> Make sure the correct ESP-IDF version, target chip, and hardware configuration are used for your specific ESP32 device.


## Build from Source
Clone this repository and open it in an appropriate ESP-IDF development environment.
```bash
git clone https://github.com/prodigylinda/ESP32_deskdog_ROM.git
cd ESP32_deskdog_ROM
```

You can then build and flash the firmware using the appropriate ESP-IDF commands for your target board.
For detailed development and build instructions, please refer to the original [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) project.

## Disclaimer
This is a personal modification of the original open-source project.
**ESP32 DeskDog ROM is not an official XiaoZhi product or official XiaoZhi firmware.**
Please make sure your hardware is compatible before flashing the firmware. The author is not responsible for damage or malfunction caused by incorrect flashing, incompatible hardware, or improper use.

## License
This project is based on the open-source [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) project.
Please refer to the original repository for its license and licensing requirements.
