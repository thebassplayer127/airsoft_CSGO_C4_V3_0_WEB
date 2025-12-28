// State.h
// VERSION: 3.7.0
// FIXED: Removed Loop from Doom Mode (Audio is long enough)

#pragma once
#include "Config.h"
#include "Sounds.h"
#include "Hardware.h"
#include "ShellEjector.h"

// Forward Declaration
void wsSendJson(const String& json);
#include "C4Net.h"

// --- GLOBAL FLAGS ---
extern bool doomModeActive;

// Game states
enum PropState { 
  STANDBY, AWAIT_ARM_TOGGLE, PROP_IDLE, ARMING, ARMED, DISARMING_KEYPAD, 
  DISARMING_MANUAL, DISARMING_RFID, DISARMED, PRE_EXPLOSION, EXPLODED, 
  EASTER_EGG, EASTER_EGG_2,
  CONFIG_MODE
};

// Config/menu states
enum ConfigState {
  MENU_MAIN, MENU_SET_BOMB_TIME, MENU_SET_MANUAL_TIME, MENU_SET_RFID_TIME,
  MENU_SUDDEN_DEATH_TOGGLE, MENU_DUD_SETTINGS, MENU_DUD_CHANCE,
  MENU_VIEW_RFIDS, MENU_ADD_RFID, MENU_CLEAR_RFIDS_CONFIRM, MENU_NET_FORGET_CONFIRM, MENU_SAVE_EXIT,
  MENU_NETWORK, MENU_NET_ENABLE, MENU_NET_SERVER_MODE, MENU_NET_IP, MENU_NET_PORT, MENU_NET_MASTER_IP,
  MENU_NET_WIFI_SETUP, MENU_NET_SAVE_BACK, MENU_NETWORK_2, MENU_NET_APPLY_NOW
};

extern PropState currentState;
extern ConfigState currentConfigState;

extern uint32_t bombArmedTimestamp;
extern uint32_t disarmStartTimestamp;
extern uint32_t lastBeepTimestamp;
extern uint32_t stateEntryTimestamp;
extern uint32_t lastStarPressTime;
extern bool ledIsOn;
extern bool displayNeedsUpdate;
extern int nextTrackToPlay;
extern int configMenuIndex;
extern char configInputBuffer[CONFIG_INPUT_MAX];
extern int rfidViewIndex;
extern char enteredCode[CODE_LENGTH + 1];
extern char activeArmCode[CODE_LENGTH + 1];
extern const char* MASTER_CODE;

inline const char* getStateName(PropState state) {
  switch (state) {
    case STANDBY: return "STANDBY";
    case AWAIT_ARM_TOGGLE: return "AWAIT_ARM_TOGGLE";
    case PROP_IDLE: return "PROP_IDLE";
    case ARMING: return "ARMING";
    case ARMED: return "ARMED";
    case DISARMING_KEYPAD: return "DISARMING_KEYPAD";
    case DISARMING_MANUAL: return "DISARMING_MANUAL";
    case DISARMING_RFID: return "DISARMING_RFID";
    case DISARMED: return "DISARMED";
    case PRE_EXPLOSION: return "PRE_EXPLOSION";
    case EXPLODED: return "EXPLODED";
    case EASTER_EGG: return "EASTER_EGG";
    case EASTER_EGG_2: return "EASTER_EGG_2";
    case CONFIG_MODE: return "CONFIG_MODE";
    default: return "UNKNOWN";
  }
}

inline void netNotifyState(const char* s) {
  String json = String("{\"type\":\"state\",\"value\":\"") + s + "\"}";
  wsSendJson(json); 
  if (strcmp(s, "DISARMED") == 0) { c4OnEnterDisarmed(); }
  else if (strcmp(s, "EXPLODED") == 0) { c4OnEnterExploded(); }
}

inline void setState(PropState newState) {
  if (currentState == newState) return;
  PropState oldState = currentState;
  currentState = newState;
  stateEntryTimestamp = millis();
  displayNeedsUpdate = true;
  netNotifyState(getStateName(newState));

  enteredCode[0] = '\0';
  
  if (newState == STANDBY || newState == PROP_IDLE) {
    doomModeActive = false;
  }

  // Silence beeper in terminal states
  switch (newState) {
    case DISARMED:
    case PRE_EXPLOSION:
    case EXPLODED:
    case STANDBY:
    case AWAIT_ARM_TOGGLE:
      beepStop();
      break;
    default: break;
  }

  switch (newState) {
    case PROP_IDLE:
      myDFPlayer.play(SOUND_ARM_SWITCH_ON);
      break;

    case STANDBY:
      if (oldState != AWAIT_ARM_TOGGLE) {
        myDFPlayer.play(SOUND_ARM_SWITCH_OFF);
      }
      break;

    case DISARMING_MANUAL:
    case DISARMING_RFID:
      disarmStartTimestamp = millis();
      myDFPlayer.play(SOUND_DISARM_BEGIN);
      break;

    case DISARMED:
      myDFPlayer.play(SOUND_DISARM_SUCCESS_1);
      nextTrackToPlay = SOUND_DISARM_SUCCESS_2; 
      break;

    case PRE_EXPLOSION:
      // FIX: Ensure clean audio transition
      // We aren't using loop() anymore, but stopping ensures the long track cuts immediately.
      myDFPlayer.stop(); 
      delay(50); 
      
      doomModeActive = false; 
      
      myDFPlayer.play(SOUND_DETONATION_NEW);
      startShellEjectorSequence(); 
      break;
      
    case EXPLODED:
      break;

    case EASTER_EGG:
      myDFPlayer.play(random(SOUND_EASTER_EGG_START, SOUND_EASTER_EGG_END + 1));
      break;

    case EASTER_EGG_2:
      myDFPlayer.play(SOUND_JUGS); 
      break;
      
    default: break;
  }
}

inline void printDetail(uint8_t type, int value) {
  if (type == DFPlayerPlayFinished) {
    Serial.printf("Track %d Finished!\n", value);
    
    // Safety: If we are already exploded, ignore further audio events 
    if (currentState == EXPLODED) return;

    // 1. Doom Logic
    if (doomModeActive && value == SOUND_DOOM_SLAYER) {
      // CHANGED: Use play() instead of loop() since track is long enough
      myDFPlayer.play(SOUND_DOOM_MUSIC); 
    }

    // 2. Detonation -> Check for Dud
    if (value == SOUND_DETONATION_NEW) {
      bool isDud = false;
      if (settings.dud_enabled) {
        if (random(0, 100) < settings.dud_chance) isDud = true;
      }

      if (isDud) {
        myDFPlayer.play(SOUND_DUD_FAIL); 
      } else {
        setState(EXPLODED); 
        nextTrackToPlay = 0;
      }
      return; 
    }

    // 3. Dud Sound Finished
    if (value == SOUND_DUD_FAIL) {
       setState(EXPLODED); 
       nextTrackToPlay = 0;
       return;
    }

    // 4. Easter Egg 2 (Jugs)
    if (currentState == EASTER_EGG_2 && value == SOUND_JUGS) {
       bombArmedTimestamp = millis();
       myDFPlayer.play(SOUND_BOMB_PLANTED);
       c4OnEnterArmed();
       setState(ARMED);
       nextTrackToPlay = 0;
       return;
    }
    
    // 5. Standard chaining
    int track = nextTrackToPlay; nextTrackToPlay = 0;
    if (track != 0) {
      myDFPlayer.play(track);
    }
  }
}