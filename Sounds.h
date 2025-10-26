// Sounds.h
//VERSION: 3.0.0
//10.26.2025

#pragma once
// MP3 track indices
#define SOUND_BOMB_PLANTED        1
#define SOUND_MENU_NAV            3
#define SOUND_EXPLOSION_PRE       4
#define SOUND_EXPLOSION_TIMESUP   5
#define SOUND_EXPLOSION_MAIN      6
#define SOUND_ARM_SWITCH          7
#define SOUND_DISARM_BEGIN        8
#define SOUND_DISARM_COMPLETE     9
#define SOUND_BOMB_DEFUSED        10
#define SOUND_EASTER_EGG_START    11
#define SOUND_EASTER_EGG_END      14
#define SOUND_MENU_CONFIRM        15
#define SOUND_MENU_CANCEL         16
#define SOUND_DETONATION_NEW      17 // C4_Detonate.mp3
#define SOUND_EASTER_EGG_2_START  18 // Jugs.mp3
#define SOUND_EASTER_EGG_2_END    18 // (for random range)
#define SOUND_EASTER_EGG_JUGS     18 // Alias for the first one
#ifndef SOUND_INVALID_CARD
  #define SOUND_INVALID_CARD SOUND_MENU_CANCEL
#endif
