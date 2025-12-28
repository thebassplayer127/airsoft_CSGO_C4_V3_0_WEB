// README.h
//VERSION: 3.0.0

//
7355608	Master Code (CS:GO): Arms the bomb but triggers the EASTER_EGG state. Plays a random "hidden" sound track (Tracks 11–14) instead of the standard arming noise.
5318008	Calculator Joke: Triggers EASTER_EGG_2. Plays the "Jugs" sound effect (Track 18) and then proceeds to arm the bomb normally.
0451	Immersive Sim Ref: Plays the "Access Denied" sound (from Deus Ex/Bioshock) without arming.
14085	MGS Codec: Plays the Metal Gear Solid "Alert (!)" sound without arming.
0000000	Lazy Code: Mocks the user by displaying "TOO EASY" and playing an error beep.
666	Sudden Death Mode: Arms the bomb instantly with a 10-second timer for a tie-breaker round.

# C4 Prop — Header-Only + Network (Arduino IDE)

**Version:** v3.0.0

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
