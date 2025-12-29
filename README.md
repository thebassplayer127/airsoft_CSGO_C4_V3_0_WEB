// README.h
//VERSION: 3.0.0

//
Airsoft CS:GO C4 Prop (Version 4.2.0)

A fully functional Counter-Strike: Global Offensive C4 Bomb prop for Airsoft and Paintball games. This project runs on an Arduino Nano ESP32 (S3) and features a 4x3 keypad, LCD display, RFID reader, MP3 audio, and programmable game modes.

🌟 Key Features

CS:GO Authentic: 7-digit arming code (7355608), realistic beeps, and explosion effects.

Multiple Disarm Methods:

Manual: Hold the disarm button for a set time (Default: 5s/10s).

RFID: Swipe a programmed NFC card/fob for a faster disarm (Default: 5s).

Code: Enter the arming code to disarm instantly.

Game Modes: Standard, Sudden Death, and various "Pop Culture" scenarios.

WiFi Scoreboard: Connects to a central scoreboard server to broadcast plant/defuse status in real-time.

Config Menu: Adjust timers, add RFID tags, and toggle modes directly on the device (Hold * on boot).

🎮 How to Play

1. Planting (Arming)

Flip the Arm Switch ON.

Enter the 7-digit code: 7355608.

Press # to confirm.

Note: The bomb must be "Planted" (magnet sensor detected) before it can be armed. (if installed)

2. Defusing

Button: Hold the Disarm Button. A progress bar will appear. Do not let go until "BOMB DEFUSED" appears.

RFID: Place a registered card/tag on the reader.

Keypad: Type the code used to plant the bomb.

🥚 Easter Eggs & Special Codes

Enter these codes instead of the standard arming code to trigger unique sounds or game modes.

Code         Effect / Mode        Description

7355608       Standard Arm        The classic CS:GO experience.

5318008       "Jugs" Mode         Plays a funny sound clip, then arms normally.

0451        Deus Ex / Bioshock    Arms the bomb with the "Access Granted" sound.

14085       Metal Gear Solid      Arms the bomb with the "!" Alert sound.

666666       DOOM Mode            Chaotic LED effects. Plays Doom soundtrack. Arms immediately.

501         Star Wars Mode        Plays Star Wars theme. Keypad acts as Lightsaber soundboard.

1138        THX Mode              Plays the deep THX "Deep Note" sound, then arms.

007       James Bond Mode         Timer set to 1m 45s. Plays Bond Theme.

1984      Terminator Mode Plays   "Hasta la vista". Special win/loss audio logic.

8675309    Jenny Mode              Timer set to 3m 42s. Plays the song "867-5309".

3141592    Pi (Nerd) Mode          Arms with a "Nerd" sound effect.

7777777     Jackpot Mode           Arms with a Slot Machine payout sound.

12345        Spaceballs           Fails to arm. Mocks the user for using a "stupid combination".

0000000       Too Easy            Fails to arm. Displays "TOO EASY" on screen.


⚔️ Special Mechanics

Sudden Death Mode: (Toggle in Menu) Flipping the Arm Switch instantly arms the bomb (no code needed). Flipping it back disarms it. Used for fast-paced tie-breakers.

Dud Bomb: (Toggle in Menu) Small random chance (default 5%) that the bomb will "fizzle" (play a sad trombone/Cartman sound) instead of detonating when the timer hits 00:00.

Star Wars Lightsaber: When in Star Wars Mode (Code 501), pressing any key on the keypad while armed plays a random lightsaber swing sound.

🛠️ Configuration Menu

To enter the Admin Menu: Hold * while powering on the device.

Bomb Time: Set explosion countdown (milliseconds).

Manual/RFID Time: Set how long it takes to defuse.

Sudden Death: Toggle ON/OFF.

Dud Settings: Enable/Disable and set failure %.

RFID Tags: View, Add, or Clear trusted tags.

Network: Configure WiFi SSID/Pass (via Captive Portal) and Scoreboard IP.

🔌 Hardware Setup (Nano ESP32)

Keypad: Rows on D2-D5, Cols on D6-D8.

LCD: I2C (SDA/SCL).

RFID (RC522): SPI (SS/SDA on D10).

Audio (DFPlayer): TX/RX on Serial0.

LEDs (NeoPixel): Data Pin A3 (or defined pin).

Servo: Pin A2.

Plant Sensor: Pin A3 (Hall Effect).

📂 Sound Files (SD Card)

Ensure files are added one at a time, in order, to the root directory. The dfplayer reads files in the order they were added, NOT by any naming logic. Stupid fucking thing.

## Key Features
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
