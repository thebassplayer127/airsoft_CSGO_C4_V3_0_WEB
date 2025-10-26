// README.h
//VERSION: 2.0.0

# C4 Prop — Header-Only + Network (Arduino IDE)

**Version:** v2.0.0

## Structure
Place in your Arduino **sketchbook** and open `C4_Prop_HeaderOnly_Net.ino`.

- `C4_Prop_HeaderOnly_Net.ino` – setup/loop & global definitions
- `Pins.h`, `Sounds.h`, `Config.h`, `Utils.h`, `Hardware.h`
- `Network.h` – Wi‑Fi (WiFiManager), mDNS, WebSocket client
- `State.h`, `Display.h`, `Game.h`

## New Features
- Wi‑Fi with **WiFiManager** captive portal (`C4Prop-Setup`) to join networks.
- mDNS discovery: resolves **scoreboard.local** (device advertises as **c4prop.local**).
- WebSocket **client** to scoreboard (`ws://<host>:<port>/ws`, default port `8080`).
- Config menu → **Network**: enable/disable Wi‑Fi, choose **mDNS** vs **Static IP**, set **Scoreboard IP**, **Port**, **Master IP**, run **Wi‑Fi Setup (Portal)**.
- **Fail‑safe:** Hold **`#` at boot** to disable Wi‑Fi for this session and run offline.
- Non‑blocking networking; gameplay never stalls if the scoreboard is offline.

## Libraries (Library Manager)
- WiFi (ESP32 core, built‑in)
- ESPmDNS (ESP32 core, built‑in)
- **WiFiManager** (tzapu)
- **arduinoWebSockets** (Markus Sattler)
- FastLED, MFRC522, DFRobotDFPlayerMini, Bounce2, Keypad, **hd44780** (Bill Perry)

## Notes
- DFPlayer stays on **Serial0** (Arduino Nano ESP32) per your wiring.
- LED current is **not limited**; brightness via `NEOPIXEL_BRIGHTNESS`.
- Version appears on boot and in the config menu header.
