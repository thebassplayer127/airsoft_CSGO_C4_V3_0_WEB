TACTICAL C4 SIMULATOR (V3.7) - FIELD MANUAL

⚡ 1-PAGE QUICK START GUIDE

DEFAULT ARMING CODE: 7355608

1. POWER ON

Flip the main power switch (battery). The screen will show the version number and then enter STANDBY or IDLE.

2. PLANTING THE BOMB (ARMING)

Place the Unit: You must place the unit on the designated "Plant Spot" (Magnet) if a sensor is installed.

Arm Switch: Flip the ARM SWITCH to the ON position.

Enter Code: Type 7 3 5 5 6 0 8 on the keypad.

Confirm: Press #.

Audio: "Bomb has been planted."

Visual: Timer starts counting down. LED flashes RED.

3. DEFUSING

There are three ways to stop the countdown:

METHOD A (The Button): Press and HOLD the blue Disarm Button.

A progress bar will appear.

DO NOT LET GO until the screen says "BOMB DEFUSED".

METHOD B (The Code): Type the same code used to arm it (7355608) on the keypad.

METHOD C (RFID Card): Tap a registered Admin or User card against the RFID reader.

4. RESETTING

Once the round is over (Exploded or Defused):

Flip the ARM SWITCH to OFF.

Press the Disarm Button once if the screen is stuck.

The unit returns to STANDBY.

📖 TABLE OF CONTENTS

Hardware Overview

Standard Gameplay

Special Game Modes

Configuration Menu (Admin)

Network & WiFi Setup

Troubleshooting

CHAPTER 1: HARDWARE OVERVIEW

LCD Screen: Displays timer, status, and menus.

Keypad: Used for entering codes and menu navigation.

Arm Switch (Toggle): A toggle switch. UP is ON, DOWN is OFF. Used to initiate the arming sequence.

Disarm Button (Push): A momentary button. Used for manual defusal or resetting the round.

Status LED: Large LED that indicates game state (Yellow=Idle, Red=Armed, Green=Defused, Blue=Disarming).

RFID Reader: Hidden internal sensor for keycards.

Plant Sensor: (Optional) A sensor on the bottom of the unit. Requires a magnet to be present to allow arming.

CHAPTER 2: STANDARD GAMEPLAY

The Plant Requirement

If your unit is equipped with a Plant Sensor, the bomb cannot be armed unless it is physically placed on a target containing a magnet.

Error Message: If you try to arm while holding the bomb, the screen will read: ERROR: MUST PLANT ON SITE FIRST!

The Timer

By default, the bomb is set to 45 Seconds. As time runs out, the beeping and LED flashing will speed up (simulating the CS:GO "stress curve").

The Explosion

When the timer hits 00:00:

Audio: A massive detonation sound plays.

Visual: The LEDs turn solid RED (or strobe, if enabled).

Physical: If the Shell Ejector is installed, the top mechanism will physically "pop" open.

The Defuse Time

Manual (Button): Takes 10 seconds by default.

RFID (Card): Takes 5 seconds by default.

Code: Instant.

CHAPTER 3: SPECIAL GAME MODES & CODES

Instead of the standard 7355608, you can enter special codes during the Arming phase to activate different modes.

☠️ Sudden Death Mode

How to Activate: Enable in Admin Menu (See Chapter 4).

Behavior: No code is required!

Flip Arm Switch ON -> Bomb Arms immediately.

Flip Arm Switch OFF -> Bomb Disarms immediately.

Use Case: Fast-paced elimination rounds.

🎬 "Pop Culture" Modes

Enter these codes instead of the standard code to change the sound effects and timer logic for that round.

Code

Mode Name

Description

501

Star Wars

Plays Imperial March. Keypad buttons make Lightsaber sounds.

666666

DOOM

Chaotic LED fire effects. Plays Heavy Metal music. Arms instantly.

007

James Bond

Timer set to 1m 45s. Plays Goldeneye Theme.

1984

Terminator

Special "Hasta La Vista" sounds.

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

Fails to arm. Mocks the user for a "stupid combination".

🎲 Dud Bomb

How to Activate: Enable in Admin Menu.

Behavior: There is a random chance (e.g., 5%) that when the timer hits 00:00, the bomb will fail to explode and play a "Sad Trombone" sound instead.

CHAPTER 4: CONFIGURATION MENU (ADMIN)

To change settings, you must enter the Config Mode.

Entering Config Mode

Turn the device OFF.

Press and HOLD the * key on the keypad.

Turn the device ON while holding *.

Release when you see the "CONFIG" menu.

Navigation Controls

2: Scroll UP

8: Scroll DOWN

#: ENTER / SELECT / SAVE

*: BACK / CANCEL

Menu Options

Bomb Time: Set countdown duration in milliseconds (e.g., 45000 = 45 seconds).

Manual Disarm: Set how long the button must be held (ms).

RFID Disarm: Set how long the card must be held (ms).

Sudden Death: Toggle ON/OFF.

Dud Settings: Toggle ON/OFF and set percentage chance.

RFID Tags:

View: See registered tag IDs.

Add: Select "Add New", then tap a card to register it.

Clear: Erase all stored cards.

Settings & Servo:

Servo: Enable/Disable the physical ejector. Set angles.

Easter Eggs: Enable/Disable the special codes.

Strobe: Toggle the white flashing light on explosion.

Network: (See Chapter 5).

Save & Exit: Saves changes to memory and reboots.

CHAPTER 5: NETWORK & WIFI SETUP

This device can connect to a Wi-Fi network to send game stats to a Scoreboard Server.

Connecting to Wi-Fi

Enter Config Mode (Hold * on boot).

Go to Network -> Page 2 -> WiFi Setup.

The screen will display AP: C4Prop-Setup.

On your phone/laptop, connect to the WiFi network named C4Prop-Setup.

A window should pop up (Captive Portal). If not, go to 192.168.4.1 in your browser.

Select your local WiFi network and enter the password.

The device will save and reboot.

Setting the Scoreboard IP

If you are running a scoreboard server, you need to tell the bomb where it is.

In Config Mode, go to Network -> IP.

Typing the IP:

Use number keys 0-9.

Use * to insert a dot (.).

Use # to Save.

Example: To type 192.168.1.50, press: 192 * 168 * 1 * 50 #.

Emergency Offline Mode

If the WiFi is down and the bomb is trying to connect (causing delays on boot), you can force it offline for one session:

Hold # while turning the device ON.

The LEDs will pulse Yellow, indicating WiFi is disabled for this game.

CHAPTER 6: TROUBLESHOOTING

Problem

Possible Cause

Solution

Device won't Arm

Plant Sensor

Ensure the device is placed on the magnetic plant spot.

"System Offline"

Arm Switch

Flip the Arm Switch to OFF, then back to ON.

Keypad is sluggish

Network Lag

The device might be trying to connect to a missing WiFi. Reboot holding # to force offline mode.

Servo hums/buzzes

Angle Settings

Go to Config -> Settings -> Servo. Adjust Start/End angles slightly (e.g., change 0 to 5).

No Sound

Volume/Card

Check volume knob (if present). Ensure SD card files are named correctly (0001.mp3, etc.).

Cannot enter IP

Wrong Key

Remember: In the IP menu, the * key types a dot.

Manual generated for Firmware v3.7.1