// Sounds.h
// VERSION: 4.3.0
// ADDED: Track 47 (SOUND_DISARM_LOOP) to expanded list

#pragma once

// --- MP3 TRACK MAPPING ---
// 1..47 matches your updated list
#define SOUND_BOMB_PLANTED        1  // played after setting code and timer starts
#define SOUND_T_WIN               2  // Not used
#define SOUND_KEY_PRESS           3  // Admin menu click
#define SOUND_BOMB_CHARGE         4  // Not used
#define SOUND_EXPLOSION_PRE       5  // Not used
#define SOUND_EXPLOSION           6  // Not used
#define SOUND_BOMB_ACTIVE         7  // Arm Switch ON
#define SOUND_DISARM_BEGIN        8  // should trigger when the disarm button is pressed or RFID card is used successfully
#define SOUND_DISARM_DEACTIVATE   9  // Disarm Success/Switch OFF
#define SOUND_BOMB_DEFUSED        10 // Played after Track 9
#define SOUND_TAZER               11 // Easter Egg 1
#define SOUND_HEADSHOT            12 // Easter Egg 2
#define SOUND_KAZOO_CHEER         13 // Easter Egg 3
#define SOUND_TAZER_KAZOO         14 // Easter Egg 4
#define SOUND_OH_YEAH             15 // Menu Confirm
#define SOUND_LAME                16 // Menu Cancel
#define SOUND_DETONATION_NEW      17 // Detonation
#define SOUND_JUGS                18 // 5318008
#define SOUND_DUD_FAIL            19 // Dud sound
#define SOUND_SOM_BITCH           20 // 0451
#define SOUND_MGS_ALERT           21 // 14085
#define SOUND_DOOM_SLAYER         22 // 666666 (Track 1 of 2)
#define SOUND_RIP_TEAR            23 // 666666 (Track 2 of 2) - WAS MISSING/NAMED WRONG
#define SOUND_BIOSHOCK            24 // Invalid RFID

// --- NEW EXPANDED LIST (25-47) ---
#define SOUND_THX                 25 // 1138
#define SOUND_JENNY               26 // 8675309 (3m42s)
#define SOUND_NERD                27 // 3141592 (Pi)
#define SOUND_HASTA_2             28 // 1984 (Terminator)
#define SOUND_JACKPOT             29 // 7777777
#define SOUND_SPACEBALLS          30 // 12345
#define SOUND_BOND_INTRO          31 // 007
#define SOUND_COME_W_ME           32 // Terminator Alt
#define SOUND_ILL_BE_BACK         33 // Terminator Detonate
#define SOUND_IN_3_YRS            34 // Terminator Skynet
#define SOUND_NOT_KILL_ANYONE     35 // Terminator Defuse
#define SOUND_BOND_THEME          36 // James Bond Theme
#define SOUND_STAR_WARS_THEME     37 // Star Wars Theme (5m50s)
#define SOUND_POWER_LIGHTSABER    38 // Lightsaber Power On
#define SOUND_LIGHTSABER_1        39 // Swing 1
#define SOUND_LIGHTSABER_2        40 // Swing 2
#define SOUND_LIGHTSABER_3        41 // Swing 3
#define SOUND_LIGHTSABER_4        42 // Swing 4
#define SOUND_LIGHTSABER_5        43 // Swing 5
#define SOUND_MAJOR_ASSHOLE       44 // Spaceballs unused
#define SOUND_AINT_FOUND_SHIT     45 // Spaceballs unused
#define SOUND_REKT_NERD           46 // Unused
#define SOUND_DISARM_LOOP         47 // Continuous noise while disarming

// --- LOGICAL ALIASES ---
#define SOUND_MENU_NAV            SOUND_KEY_PRESS
#define SOUND_MENU_CONFIRM        SOUND_OH_YEAH
#define SOUND_MENU_CANCEL         SOUND_LAME
#define SOUND_INVALID_CARD        SOUND_BIOSHOCK
#define SOUND_ARM_SWITCH_ON       SOUND_BOMB_ACTIVE
#define SOUND_ARM_SWITCH_OFF      SOUND_DISARM_DEACTIVATE

// Disarm Sequence
#define SOUND_DISARM_SUCCESS_1    SOUND_DISARM_DEACTIVATE 
#define SOUND_DISARM_SUCCESS_2    SOUND_BOMB_DEFUSED

// Ranges
#define SOUND_EASTER_EGG_START    11
#define SOUND_EASTER_EGG_END      14
#define SOUND_SWING_START         39
#define SOUND_SWING_END           43

// --- BEEP TONES ---
// Note: We use tone() for the countdown beeps, not mp3s, 
// to ensure perfect timing with the LED strobe.
#define BEEP_TONE_FREQ     3000
#define BEEP_TONE_DURATION_MS 100