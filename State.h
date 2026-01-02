// State.h
// VERSION: 5.0.0
// FIXED: Servo Trigger Flag Reset
// FIXED: Externs for Servo Logic

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
extern bool easterEggActive;

// Fix 1: Make this extern so we can reset it in setState
extern bool servoTriggeredThisExplosion; 

// Game states
enum PropState { 
  STANDBY, AWAIT_ARM_TOGGLE, PROP_IDLE, ARMING, ARMED, DISARMING_KEYPAD, 
  DISARMING_MANUAL, DISARMING_RFID, DISARMED, PRE_EXPLOSION, EXPLODED, 
  EASTER_EGG, EASTER_EGG_2,
  CONFIG_MODE,
  STARWARS_PRE_GAME,
  PROP_DUD  // New State for Dud Logic
};

// Config/menu states
enum ConfigState {
  MENU_MAIN, MENU_SET_BOMB_TIME, MENU_SET_MANUAL_TIME, MENU_SET_RFID_TIME,
  MENU_SUDDEN_DEATH_TOGGLE, MENU_DUD_SETTINGS, MENU_DUD_CHANCE,
  MENU_EXTRAS_SUBMENU, 
  MENU_TOGGLE_EASTER_EGGS, MENU_TOGGLE_STROBE,
  MENU_SERVO_SETTINGS, MENU_SERVO_TOGGLE, MENU_SERVO_START_ANGLE, MENU_SERVO_END_ANGLE,
  MENU_VIEW_RFIDS, MENU_ADD_RFID, MENU_CLEAR_RFIDS_CONFIRM, MENU_NET_FORGET_CONFIRM, MENU_SAVE_EXIT, MENU_EXIT_NO_SAVE,
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
    case PROP_DUD: return "PROP_DUD";
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
  easterEggActive = false; 
  
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

  // --- FIX 1: Reset Servo Flag ---
  // Ensure servo can fire again on the next game
  if (newState == STANDBY || newState == ARMED) {
    servoTriggeredThisExplosion = false;
  }

  switch (newState) {
    case DISARMED:
    case PRE_EXPLOSION:
    case EXPLODED:
    case STANDBY:
    case AWAIT_ARM_TOGGLE:
    case PROP_DUD:
      beepStop();
      break;
    default: break;
  }

  switch (newState) {
    case PROP_IDLE:
      safePlay(SOUND_ARM_SWITCH_ON);
      break;

    case STANDBY:
      if (oldState != AWAIT_ARM_TOGGLE) {
        safePlay(SOUND_ARM_SWITCH_OFF);
      }
      break;
      
    case STARWARS_PRE_GAME:
      safePlay(SOUND_POWER_LIGHTSABER); 
      break;

    case DISARMING_MANUAL:
    case DISARMING_RFID:
      disarmStartTimestamp = millis();
      safePlay(SOUND_DISARM_BEGIN);
      break;

    case DISARMED:
      if (terminatorModeActive) {
         safePlay(SOUND_NOT_KILL_ANYONE); 
         nextTrackToPlay = SOUND_DISARM_SUCCESS_2; 
      } 
      else {
         safePlay(SOUND_DISARM_SUCCESS_1);
         nextTrackToPlay = SOUND_DISARM_SUCCESS_2; 
      }
      break;

    case PRE_EXPLOSION: {
      bool isDud = false;
      if (settings.dud_enabled && !terminatorModeActive) {
        if (random(1, 101) <= settings.dud_chance) isDud = true;
      }

      if (isDud) {
        setState(PROP_DUD); 
        return; 
      }

      myDFPlayer.stop(); 
      delay(50); 
      
      doomModeActive = false; 
      
      if (terminatorModeActive) {
         safePlay(SOUND_ILL_BE_BACK); 
      }
      else {
         safePlay(SOUND_DETONATION_NEW);
      }
      
      // NOTE: Servo is NO LONGER triggered here instantly.
      // It is handled in the main loop() or Display::updateLeds() timing
      // to sync with the strobe (approx 4.5s in).
    } break;
      
    case EXPLODED:
      break;
    
    case PROP_DUD:
      myDFPlayer.stop();
      delay(50);
      safePlay(SOUND_DUD_FAIL);
      break;

    case EASTER_EGG:
      easterEggActive = true; 
      safePlay(random(SOUND_EASTER_EGG_START, SOUND_EASTER_EGG_END + 1));
      bombArmedTimestamp = millis();
      c4OnEnterArmed();
      setState(ARMED); 
      break;

    case EASTER_EGG_2:
      safePlay(SOUND_JUGS); 
      break;
      
    default: break;
  }
}

inline void printDetail(uint8_t type, int value) {
  if (type == DFPlayerPlayFinished) {
    Serial.printf("Track %d Finished!\n", value);
    
    if (currentState == EXPLODED) return;

    if (doomModeActive && value == SOUND_DOOM_SLAYER) {
      safePlay(SOUND_RIP_TEAR); 
    }

    if (bondModeActive && value == SOUND_BOND_INTRO) {
      safePlay(SOUND_BOND_THEME);
    }

    if (value == SOUND_DETONATION_NEW) {
      setState(EXPLODED); 
      nextTrackToPlay = 0;
      return; 
    }

    if (value == SOUND_ILL_BE_BACK) {
        setState(EXPLODED);
        nextTrackToPlay = 0;
        return;
    }

    if (value == SOUND_DUD_FAIL) {
       setState(STANDBY); 
       nextTrackToPlay = 0;
       return;
    }

    if (currentState == EASTER_EGG_2 && value == SOUND_JUGS) {
       bombArmedTimestamp = millis();
       safePlay(SOUND_BOMB_PLANTED);
       c4OnEnterArmed();
       setState(ARMED);
       nextTrackToPlay = 0;
       return;
    }
    
    if (value >= SOUND_EASTER_EGG_START && value <= SOUND_EASTER_EGG_END) {
        easterEggActive = false;
    }

    int track = nextTrackToPlay; nextTrackToPlay = 0;
    if (track != 0) {
      safePlay(track);
    }
  }
}