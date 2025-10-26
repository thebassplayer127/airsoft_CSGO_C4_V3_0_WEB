// State.h
//VERSION: 3.0.0
//10.26.2025

#pragma once
#include "Config.h"
#include "Sounds.h"
#include "Hardware.h"
// DO NOT include Network.h here (breaks include order)
// We only need the declaration for wsSendJson used below:
void wsSendJson(const String& json);

#include "C4Net.h"

// Game states
enum PropState { 
  STANDBY, AWAIT_ARM_TOGGLE, PROP_IDLE, ARMING, ARMED, DISARMING_KEYPAD, 
  DISARMING_MANUAL, DISARMING_RFID, DISARMED, PRE_EXPLOSION, EXPLODED, EASTER_EGG, EASTER_EGG_2,
  CONFIG_MODE
};

// Config/menu states
enum ConfigState {
  MENU_MAIN, MENU_SET_BOMB_TIME, MENU_SET_MANUAL_TIME, MENU_SET_RFID_TIME,
  MENU_VIEW_RFIDS, MENU_ADD_RFID, MENU_CLEAR_RFIDS_CONFIRM, MENU_NET_FORGET_CONFIRM, MENU_SAVE_EXIT,
  // Network submenu
  MENU_NETWORK, MENU_NET_ENABLE, MENU_NET_SERVER_MODE, MENU_NET_IP, MENU_NET_PORT, MENU_NET_MASTER_IP,
  MENU_NET_WIFI_SETUP, MENU_NET_SAVE_BACK,
  // NEW in 2.0.1/2
  MENU_NETWORK_2,
  MENU_NET_APPLY_NOW
};

// Externs defined in .ino
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
  wsSendJson(json); // comes from Network.h, but only needs a declaration here
   // Also tell the scoreboard about bomb-critical transitions:
  if      (strcmp(s, "DISARMED") == 0) { c4OnEnterDisarmed(); }
  else if (strcmp(s, "EXPLODED") == 0) { c4OnEnterExploded(); }
}

inline void setState(PropState newState) {
  if (currentState == newState) return;
  currentState = newState;
  stateEntryTimestamp = millis();
  displayNeedsUpdate = true;
  Serial.printf("State changed to: %s\n", getStateName(newState));
  netNotifyState(getStateName(newState));

  enteredCode[0] = '\0';
   // Only silence the beeper in terminal/non-tension states
  switch (newState) {
    case DISARMED:
    case PRE_EXPLOSION:
    case EXPLODED:
    case STANDBY:
    case AWAIT_ARM_TOGGLE:
      beepStop();
      break;
    default: /* keep beeper running */ break;
  }

  switch (newState) {
    case PROP_IDLE:
      myDFPlayer.play(SOUND_ARM_SWITCH);
      break;
    case DISARMING_MANUAL:
    case DISARMING_RFID:
      disarmStartTimestamp = millis();
      myDFPlayer.play(SOUND_DISARM_BEGIN);
      break;
    case DISARMED:
      myDFPlayer.play(SOUND_DISARM_COMPLETE);
      nextTrackToPlay = SOUND_BOMB_DEFUSED;
      break;
    case PRE_EXPLOSION:
      myDFPlayer.play(SOUND_DETONATION_NEW);
      break;
    case EASTER_EGG:
      myDFPlayer.play(random(SOUND_EASTER_EGG_START, SOUND_EASTER_EGG_END + 1));
      break;

    case EASTER_EGG_2:
      myDFPlayer.play(random(SOUND_EASTER_EGG_2_START, SOUND_EASTER_EGG_2_END + 1));
      break;
    default: break;
  }
}

inline void printDetail(uint8_t type, int value) {
  switch (type) {
    case DFPlayerPlayFinished: {
      Serial.printf("Track %d Finished!\n", value);

      // Handle new all-in-one explosion sound
      if (value == SOUND_DETONATION_NEW) {
        setState(EXPLODED); // Go to EXPLODED after new track finishes
        nextTrackToPlay = 0;
        break; // Stop further processing
      }

      // Handle Easter Egg 2 finish
      // This checks if we are in EASTER_EGG_2 state AND the easter egg 2 sound just finished
      if (currentState == EASTER_EGG_2 && 
          (value >= SOUND_EASTER_EGG_2_START && value <= SOUND_EASTER_EGG_2_END)) {
        
        // Now, perform the normal arming sequence
        bombArmedTimestamp = millis();
        myDFPlayer.play(SOUND_BOMB_PLANTED);
        c4OnEnterArmed();
        setState(ARMED);
        
        nextTrackToPlay = 0; // Clear any other pending tracks
        break; // Stop further processing
      }
      
      // Old track-chaining logic
      int track = nextTrackToPlay; nextTrackToPlay = 0;
      if (track != 0) {
        myDFPlayer.play(track);
        // Old explosion chaining logic was removed here
      }
    } break; // <-- This closes the 'case DFPlayerPlayFinished:'
    
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      break;
    default: break;
  }
}
