// State.h
// VERSION: 4.4.0
// FIXED: Easter Egg Arming, Servo Triggering

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
extern bool starWarsModeActive;
extern bool terminatorModeActive;
extern bool bondModeActive;
extern bool suddenDeathActive;

// Game states
enum PropState { 
  STANDBY, AWAIT_ARM_TOGGLE, PROP_IDLE, ARMING, ARMED, DISARMING_KEYPAD, 
  DISARMING_MANUAL, DISARMING_RFID, DISARMED, PRE_EXPLOSION, EXPLODED, 
  EASTER_EGG, EASTER_EGG_2,
  CONFIG_MODE,
  STARWARS_PRE_GAME 
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
    case STARWARS_PRE_GAME: return "STARWARS_PRE";
    default: return "UNKNOWN";
  }
}

inline void netNotifyState(const char* s) {
  String json = String("{\"type\":\"state\",\"value\":\"") + s + "\"}";
  wsSendJson(json); 
  if (strcmp(s, "DISARMED") == 0) { c4OnEnterDisarmed(); }
  else if (strcmp(s, "EXPLODED") == 0) { c4OnEnterExploded(); }
}

inline void resetSpecialModes() {
  doomModeActive = false;
  starWarsModeActive = false;
  terminatorModeActive = false;
  bondModeActive = false;
  suddenDeathActive = false;
  
  Settings temp; 
  EEPROM.get(0, temp);
  settings.bomb_duration_ms = temp.bomb_duration_ms;
}

inline void setState(PropState newState) {
  if (currentState == newState) return;
  PropState oldState = currentState;
  currentState = newState;
  stateEntryTimestamp = millis();
  displayNeedsUpdate = true;
  netNotifyState(getStateName(newState));

  enteredCode[0] = '\0';
  
  if (newState == STANDBY || newState == PROP_IDLE || newState == AWAIT_ARM_TOGGLE) {
    resetSpecialModes();
  }

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
      
    case STARWARS_PRE_GAME:
      myDFPlayer.play(SOUND_POWER_LIGHTSABER); 
      break;

    case DISARMING_MANUAL:
    case DISARMING_RFID:
      disarmStartTimestamp = millis();
      myDFPlayer.play(SOUND_DISARM_BEGIN);
      break;

    case DISARMED:
      if (terminatorModeActive) {
         myDFPlayer.play(SOUND_NOT_KILL_ANYONE); 
         nextTrackToPlay = SOUND_DISARM_SUCCESS_2; 
      } 
      else {
         myDFPlayer.play(SOUND_DISARM_SUCCESS_1);
         nextTrackToPlay = SOUND_DISARM_SUCCESS_2; 
      }
      break;

    case PRE_EXPLOSION:
      myDFPlayer.stop(); 
      delay(50); 
      
      doomModeActive = false; 
      
      if (terminatorModeActive) {
         myDFPlayer.play(SOUND_ILL_BE_BACK); 
      }
      else {
         myDFPlayer.play(SOUND_DETONATION_NEW);
      }
      
      // CRITICAL: Call Servo Trigger here. The delay logic is internal to ShellEjector
      startShellEjectorSequence(); 
      break;
      
    case EXPLODED:
      break;

    case EASTER_EGG:
      // Master Code Logic: Play sound AND transition to ARMED
      myDFPlayer.play(random(SOUND_EASTER_EGG_START, SOUND_EASTER_EGG_END + 1));
      
      // Transition to Armed immediately after sound starts (non-blocking)
      bombArmedTimestamp = millis();
      c4OnEnterArmed();
      setState(ARMED); 
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
    
    if (currentState == EXPLODED) return;

    // 1. Doom Logic
    if (doomModeActive && value == SOUND_DOOM_SLAYER) {
      myDFPlayer.play(SOUND_RIP_TEAR); 
    }

    // 2. Bond Logic
    if (bondModeActive && value == SOUND_BOND_INTRO) {
      myDFPlayer.play(SOUND_BOND_THEME);
    }

    // 3. Detonation -> Check for Dud
    if (value == SOUND_DETONATION_NEW) {
      bool isDud = false;
      if (settings.dud_enabled) {
        if (random(1, 101) <= settings.dud_chance) isDud = true;
      }

      if (isDud) {
        myDFPlayer.play(SOUND_DUD_FAIL); 
      } else {
        setState(EXPLODED); 
        nextTrackToPlay = 0;
      }
      return; 
    }

    // 4. Terminator Detonation Finished
    if (value == SOUND_ILL_BE_BACK) {
        setState(EXPLODED);
        nextTrackToPlay = 0;
        return;
    }

    // 5. Dud Sound Finished
    if (value == SOUND_DUD_FAIL) {
       setState(EXPLODED); 
       nextTrackToPlay = 0;
       return;
    }

    // 6. Easter Egg 2 (Jugs)
    if (currentState == EASTER_EGG_2 && value == SOUND_JUGS) {
       bombArmedTimestamp = millis();
       myDFPlayer.play(SOUND_BOMB_PLANTED);
       c4OnEnterArmed();
       setState(ARMED);
       nextTrackToPlay = 0;
       return;
    }
    
    // 7. Standard chaining
    int track = nextTrackToPlay; nextTrackToPlay = 0;
    if (track != 0) {
      myDFPlayer.play(track);
    }
  }
}