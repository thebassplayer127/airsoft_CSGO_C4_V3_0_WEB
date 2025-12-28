// Sounds.h
// VERSION: 3.3.0
// 12.28.2025

#pragma once

// --- MP3 TRACK MAPPING ---
// 1..24 matches your specific file list
#define SOUND_BOMB_PLANTED        1  // Timer Start
#define SOUND_T_WIN               2  // Unused
#define SOUND_KEY_PRESS           3  // Admin Menu Click
#define SOUND_BOMB_CHARGE         4  // Unused
#define SOUND_EXPLOSION_PRE       5  // Unused
#define SOUND_EXPLOSION           6  // Unused
#define SOUND_BOMB_ACTIVE         7  // Arm Switch Toggle ON
#define SOUND_DISARM_BEGIN        8  // Button Press / RFID Start
#define SOUND_DISARM_DEACTIVATE   9  // Disarm Success OR Switch Toggle OFF
#define SOUND_BOMB_DEFUSED        10 // Played AFTER Track 9 finishes (Success)
#define SOUND_TAZER               11 // Easter Egg 1
#define SOUND_HEADSHOT            12 // Easter Egg 2
#define SOUND_KAZOO_CHEER         13 // Easter Egg 3
#define SOUND_TAZER_KAZOO         14 // Easter Egg 4
#define SOUND_OH_YEAH             15 // Menu Confirm
#define SOUND_LAME                16 // Menu Cancel
#define SOUND_DETONATION_NEW      17 // Explosion
#define SOUND_JUGS                18 // 5318008 Code
#define SOUND_DUD_FAIL            19 // Dud / "Going Home"
#define SOUND_SOM_BITCH           20 // 0451 Code
#define SOUND_MGS_ALERT           21 // 14085 Code
#define SOUND_DOOM_SLAYER         22 // 666666 Code (Intro)
#define SOUND_DOOM_MUSIC          23 // 666666 Code (Loop)
#define SOUND_BIOSHOCK            24 // Invalid RFID

// --- LOGICAL ALIASES ---
#define SOUND_MENU_NAV            SOUND_KEY_PRESS
#define SOUND_MENU_CONFIRM        SOUND_OH_YEAH
#define SOUND_MENU_CANCEL         SOUND_LAME
#define SOUND_INVALID_CARD        SOUND_BIOSHOCK
#define SOUND_ARM_SWITCH_ON       SOUND_BOMB_ACTIVE
#define SOUND_ARM_SWITCH_OFF      SOUND_DISARM_DEACTIVATE

// Disarm Sequence: 9 plays first, then code in State.h chains 10
#define SOUND_DISARM_SUCCESS_1    SOUND_DISARM_DEACTIVATE 
#define SOUND_DISARM_SUCCESS_2    SOUND_BOMB_DEFUSED

// Easter Egg Random Ranges
#define SOUND_EASTER_EGG_START    11
#define SOUND_EASTER_EGG_END      14