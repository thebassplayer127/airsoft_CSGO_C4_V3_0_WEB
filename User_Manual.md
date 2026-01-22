TACTICAL C4 SIMULATOR - FIELD MANUAL

Firmware Version: 4.1.1 Designed by Andrew Florio

⚡ 1-PAGE QUICK START GUIDE

1. POWER ON

Flip the main power switch. The screen will show the version number and credits, then enter STANDBY or IDLE.

2. PLANTING THE BOMB (ARMING)

Place the Unit: If the Plant Sensor is enabled, place the unit on the magnetic "Plant Spot".

Arm Switch: Flip the ARM SWITCH to the ON position.

Enter Code: Type 7 3 5 5 6 0 8 on the keypad or any code the user wants.

Confirm: Press #.

Audio: "Bomb has been planted."

Visual: Timer starts. Status LED flashes RED.

3. DEFUSING

There are three ways to stop the countdown:

METHOD A (The Button): Press and HOLD the blue Disarm Button. Do not let go until the screen says "BOMB DEFUSED".

METHOD B (The Code): Type the arming code (7355608 or user code) on the keypad.

METHOD C (RFID Card): Tap a registered card into the slot reader.

4. RESETTING

Once the round is over (Exploded or Defused):

Flip the ARM SWITCH to OFF.

The unit returns to STANDBY.

📖 TABLE OF CONTENTS

Hardware Overview & Status Lights

Standard Gameplay

Special Game Modes & Codes

Configuration Menu (Admin)

Network & WiFi Setup

Troubleshooting

CHAPTER 1: HARDWARE OVERVIEW & STATUS LIGHTS

The Controls

LCD Screen: Displays timer, status, and menus.

Keypad: Used for codes and menu navigation.

Arm Switch: Toggle switch. UP=ON, DOWN=OFF. Initiates arming.

Disarm Button: Blue push button. Used for manual defusal.

Plant Sensor: (Optional) Magnetic sensor on bottom. Requires magnet to arm.

Status Indicators

1. Main Status LED (The "Game" Light)

This is the large external LED (or LED strip) that indicates the game state.

Color

Behavior

State

Yellow

Solid

IDLE / READY (Waiting for code)

Red

Blinking

ARMED (Counts down with the beep)

Blue

Solid

DISARMING (Button or RFID active)

Green

Solid

DEFUSED (Counter-Terrorists Win)

Red

Solid/Strobe

EXPLODED (Terrorists Win)

Deep Pink

Solid

CONFIG MODE (Admin Menu)

Purple/Orange

Toggling

DUD (Bomb failed to detonate)

2. Network LED (Onboard Chip)

This is the small RGB LED located directly on the Arduino Nano ESP32 board. It indicates WiFi/Server status.

Color

Behavior

State

Yellow

Pulsing

WiFi Disabled (Offline Mode)

Green

Solid

WiFi Connected (But not connected to Scoreboard)

Blue

Solid

ONLINE (WiFi + Scoreboard Connected)

Red

Solid

Connection Failed (Check IP/WiFi)

CHAPTER 2: STANDARD GAMEPLAY

The Plant Requirement

If the Plant Sensor is enabled in the Hardware Menu:

You MUST place the bomb on a magnetic surface to arm it.

If you try to arm while holding it, the screen will read: ERROR: MUST PLANT ON SITE FIRST!

The Timer logic

Stress Curve: As the timer counts down, the beeping and LED flashing will speed up, simulating the CS:GO stress curve.

35-Second Mark: Beeps become faster.

10-Second Mark: Beeps become rapid.

The Explosion

When the timer hits 00:00:

Audio: Massive detonation sound.

Visual: LEDs turn Red (or Strobe White if enabled).

Physical: If the Shell Ejector is enabled, the an airsoft 40mm shell will go off.

Defusing

Manual (Button): Default 15s. Hold the button. Releasing it early resets progress.

RFID (Card): Default 5s. Hold card against reader.

Code: Instant.

CHAPTER 3: SPECIAL GAME MODES & CODES

Enter these codes instead of the standard code to activate special modes.

☠️ Sudden Death Mode

Activation: Enable in Admin Menu (Sudden Death).

Behavior: NO CODE REQUIRED.

Flip Arm Switch ON -> Arms immediately.

Flip Arm Switch OFF -> Disarms immediately.

Use Case: Fast-paced elimination rounds.

🎬 "Pop Culture" Easter Eggs

Enable "Easter Eggs" in the Hardware Menu to use these.

Code

Mode Name

Description

501

Star Wars

Plays Imperial March. Keypad keys trigger Lightsaber sounds. Press # to Arm.

666666

DOOM

Chaotic "Hellfire" LEDs. Plays Heavy Metal. Arms instantly.

007

James Bond

Timer set to 1m 45s. Plays Goldeneye Theme.

1984

Terminator

"Hasta La Vista" sounds. Special win/loss audio.

8675309

Jenny

Timer set to 3m 42s. Plays the song "867-5309".

5318008

"Jugs"

Plays a funny sound clip, then arms.

0451

BioShock

Plays "Access Granted" sound.

1138

THX

Plays the massive THX "Deep Note" sound.

12345

Spaceballs

Fails to arm. Mocks user for "stupid combination".

3141592

Nerd Mode

Arms with "Nerd" sound effect.

7777777

Jackpot

Arms with Slot Machine payout sound.

🎲 Dud Bomb

Activation: Enable in Admin Menu. Set a percentage (e.g., 5%).

Behavior: When the timer hits 00:00, there is a chance the bomb will fail to explode. It will play a "Sad Trombone" sound and the LEDs will toggle Purple/Orange.

CHAPTER 4: CONFIGURATION MENU (ADMIN)

This is where you customize the device.

Entering Config Mode

Turn the device OFF.

Press and HOLD the * key.

Turn the device ON while holding *.

Release when you see the pink LED or "CONFIG" on screen.

Navigation

2: Scroll UP

8: Scroll DOWN

#: SELECT / ENTER / SAVE

*: BACK / CANCEL

📂 Main Menu Structure

1. Bomb Time

Set the countdown duration in milliseconds.

Example: 45000 = 45 Seconds. 120000 = 2 Minutes.

2. Manual Disarm

Set how long the Blue Button must be held to defuse (ms).

3. RFID Disarm

Set how long an RFID card must be held to defuse (ms).

4. Sudden Death

Toggle Sudden Death Mode ON/OFF.

5. Dud Settings

Toggle: Enable/Disable Dud mechanics.

Set Chance: Set the probability (0-100%) of a dud.

6. RFID Tags

Manage keycards.

View: Scroll through registered card IDs.

Add New: Select this, then tap a new card to register it.

Clear All: Erases all trusted cards.

7. Hardware & Audio (Submenu)

1 Audio:

Sound: Toggle all sound effects ON/OFF.

Volume: Set volume level (0-30).

2 Servo (Shell Ejector):

Svo: Enable/Disable the physical ejector.

Str: Set Start Angle (Closed position).

End: Set End Angle (Open position).

3 Sensor:

Enable/Disable the Plant Sensor requirement.

4 FX/Xtras:

Strobe: Toggle white explosion strobe.

Eggs: Enable/Disable Easter Egg codes.

8. Network (Submenu)

See Chapter 5 for details.

9. Save & Exit

IMPORTANT: You must select this to save your changes! The device will reboot.

10. Exit

Leaves the menu without saving to memory (changes strictly for this session).

CHAPTER 5: NETWORK & WIFI SETUP

This device connects to a Scoreboard Server to broadcast game events (Plant, Defuse, Explosion).

Step 1: Connect to WiFi

Enter Config Mode.

Go to Network -> 9 Next -> 6 Setup.

The screen will display AP: C4Prop-Setup.

On your phone/PC, connect to the WiFi network C4Prop-Setup.

A portal window will open. Select your local WiFi SSID and enter the password.

The device will save and allow you to exit the menu.

Step 2: Server Configuration

You must tell the bomb where the Scoreboard Server is.

Go to Network.

Mode: Select mDNS (easier) or IP (advanced).

mDNS: Looks for scoreboard.local.

IP: You manually type the server's IP address (e.g., 192.168.1.50). Use * to type a dot.

Port: Default is 8080.

Emergency Offline Mode

If you are at a field with no WiFi, the device might pause on boot trying to connect.

Force Offline: Hold # while turning the device ON.

The Onboard LED will pulse Yellow, and WiFi will be disabled for that game.

CHAPTER 6: TROUBLESHOOTING

Problem

Possible Cause

Solution

"ERROR: MUST PLANT"

Plant Sensor Enabled

Place unit on the magnet pad, or disable Sensor in Hardware menu.

Device freezes on boot

Network Search

Hold # during boot to force Offline Mode.

Servo buzzes/hums

Angle Stress

Adjust Start/End Angles in Hardware -> Servo menu (e.g., change 0 to 5).

Audio cuts out

Power/SD

Check battery charge. Ensure SD card is seated.

Cannot enter IP

Keypad

Use * to type a dot (.). Use # to save.

Status LED is Off

Standby Mode

The status LED is intentionally off in Standby to save power.

Manual generated for Firmware v4.1.0