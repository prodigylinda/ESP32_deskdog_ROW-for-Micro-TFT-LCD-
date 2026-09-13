# ESP32_deskdog_ROW-for-Micro-TFT-LCD-

A modified version of `xiaozhi-esp32` for the **ESP-Hi / DeskDog** platform with a **160 × 80 Micro TFT LCD**.

This project keeps the original DeskDog facial animation system while adding a dedicated subtitle area at the bottom of the screen for displaying AI responses.

Repository:

[ESP32_deskdog_ROW-for-Micro-TFT-LCD-](https://github.com/prodigylinda/ESP32_deskdog_ROW-for-Micro-TFT-LCD-)

---

## Features

* Keeps the original AAF facial animations
* Adds a dedicated subtitle area at the bottom of the TFT display
* Displays AI assistant responses in English
* Black subtitle background
* White text
* Short messages are automatically centered
* Long messages automatically scroll horizontally
* Previous AI subtitles are cleared when the user starts speaking
* Uses the built-in Xiaozhi font
* Does not require a second LCD refresh pass
* Designed specifically for the ESP-Hi / DeskDog 160 × 80 display

---

## Display Layout

The ESP-Hi display resolution is:

```text
160 × 80
```

The screen is divided into two areas:

```text
┌────────────────────────────┐
│                            │
│                            │
│      Facial Animation      │
│                            │
│                            │
├────────────────────────────┤
│     AI Response Text       │
└────────────────────────────┘
```

Pixel layout:

```text
Y = 0  ~ 63   Facial animation
Y = 64 ~ 79   Subtitle area
```

The bottom 16 pixels are reserved for subtitles.

---

## How It Works

The original ESP-Hi implementation decodes AAF animation frames using `anim_player` and sends the RGB565 frame directly to the LCD.

This project modifies the frame before it is sent to the display.

```text
AAF Animation
     │
     ▼
 anim_player
     │
     ▼
  OnFlush()
     │
     ▼
RenderSubtitle()
     │
     ▼
Animation + Subtitle
     │
     ▼
    LCD
```

The subtitle is drawn directly into the existing RGB565 animation framebuffer.

This is important because the LCD still receives only one framebuffer transfer per animation flush.

---

## Subtitle Behavior

### Short Messages

Short messages are automatically centered.

Example:

```text
Hello!
```

Display:

```text
┌────────────────────────────┐
│                            │
│      Facial Animation      │
│                            │
├────────────────────────────┤
│           Hello!           │
└────────────────────────────┘
```

---

### Long Messages

Messages wider than the available display area automatically scroll from right to left.

Example:

```text
Hello, how can I help you today?
```

The text continuously moves across the subtitle area.

---

## Font

The ESP-Hi configuration uses the built-in Xiaozhi font:

```text
font_noto_sans_basic_14_1
```

This is a 14 px, 1-bit font suitable for the 16 px subtitle area.

The current version is focused on:

```text
English
Numbers
Basic ASCII punctuation
```

Non-ASCII characters are currently replaced with:

```text
?
```

Chinese subtitle support may be added later using dynamic glyph loading.

---

## AI Message Handling

The subtitle system listens to messages sent through:

```cpp
SetChatMessage()
```

Only messages with:

```text
role = assistant
```

are displayed.

Example:

```cpp
display->SetChatMessage(
    "assistant",
    "Hello, how can I help you?"
);
```

When:

```text
role = user
```

is received, the previous AI subtitle is cleared.

---

## Main Modified Files

The main modifications are located in:

```text
main/boards/espressif/esp-hi/emoji_display.h
main/boards/espressif/esp-hi/emoji_display.cc
```

The subtitle implementation adds functions such as:

```text
SetSubtitle()
ResolveGlyph()
MeasureSubtitleWidthLocked()
RenderSubtitle()
SetChatMessage()
ClearChatMessages()
```

---

## Hardware

Target platform:

```text
ESP-Hi / DeskDog
```

MCU:

```text
ESP32-C3
```

Display:

```text
160 × 80 Micro TFT LCD
```

---

## Base Project

This project is based on:

```text
78/xiaozhi-esp32
```

Original repository:

https://github.com/78/xiaozhi-esp32

Development base commit:

```text
8e2899dbc9249d9961b6dafc0c59f7bd7e72644d
```

This revision was selected because it remains compatible with ESP-IDF 5.5.x.

---

## Development Environment

Recommended ESP-IDF version:

```text
ESP-IDF 5.5.2 or newer
```

Development was performed with:

```text
ESP-IDF 5.5.5
```

Target chip:

```text
esp32c3
```

---

## Build

After configuring the ESP-IDF environment, build the ESP-Hi firmware with:

```bash
python scripts/build.py espressif/esp-hi
```

Do not configure this board as a standard `esp32`.

The ESP-Hi uses:

```text
esp32c3
```

---

## Flash

Using ESP-IDF:

```bash
idf.py -p COM7 flash
```

Replace:

```text
COM7
```

with the correct serial port.

If a merged firmware image is available:

```bash
python -m esptool \
  --chip esp32c3 \
  -p COM7 \
  -b 460800 \
  write_flash 0x0 build/merged-binary.bin
```

---

## Animation Assets

ESP-Hi facial animations are stored as AAF assets.

If the device reports an error similar to:

```text
Failed to get asset data for connecting
```

the animation assets may not have been flashed correctly.

In that case, flashing the complete:

```text
merged-binary.bin
```

is recommended instead of flashing only the application partition.

---

## Current Status

* [x] Original DeskDog facial animations
* [x] Bottom subtitle area
* [x] Black subtitle background
* [x] English AI response text
* [x] Short text centering
* [x] Long text scrolling
* [x] Clear old subtitle when the user speaks
* [ ] Chinese subtitles
* [ ] Mixed Chinese/English subtitles
* [ ] Emoji text rendering
* [ ] Configurable scrolling speed
* [ ] Subtitle fade animations

---

## Future Improvements

Possible future improvements include:

* Chinese dynamic glyph support
* Mixed Chinese and English subtitles
* Configurable subtitle speed
* Subtitle timeout
* Fade-in / fade-out effects
* More subtitle styles
* Additional small-screen UI improvements

---

## Credits

Based on the original:

```text
xiaozhi-esp32
```

by the Xiaozhi ESP32 project contributors.

Original repository:

https://github.com/78/xiaozhi-esp32

This repository focuses specifically on adapting the **ESP-Hi / DeskDog Micro TFT display** for AI response subtitles.

---

## License

This project follows the licensing terms of the upstream `xiaozhi-esp32` project and its dependencies.

Please review the upstream licenses before redistribution or commercial use.
