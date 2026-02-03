// Game.h
// VERSION: 6.5.0
// UPDATE: Added Delete Logic for individual RFID cards
// FIXED: Auto-Typing persistence logic is now in State.h

#pragma once
#include <Arduino.h>
#include "State.h"
#include "Hardware.h"
#include "Display.h"
#include "Utils.h"
#include "Network.h"
#include "C4Net.h"
#include "PlantSensor.h"

// Global Flags
bool doomModeActive = false;
bool starWarsModeActive = false;
bool terminatorModeActive = false;
bool bondModeActive = false;
bool suddenDeathActive = false;
bool easterEggActive = false; 

// Auto Typing Globals
bool autoTypingActive = false;
char autoTypingTarget[CODE_LENGTH + 1];
uint32_t lastAutoTypeTime = 0;
uint32_t lastPingTime = 0; // For homing ping

// Cache for unsaved RAM settings
uint32_t stored_duration_ram = 0;

inline bool parseIpFromBuffer(const char* buf, uint32_t& out) {
  int a,b,c,d;
  if (sscanf(buf, "%d.%d.%d.%d", &a,&b,&c,&d) == 4) {
    if (a<0||b<0||c<0||d<0||a>255||b>255||c>255||d>255) return false;
    out = (uint32_t)( (a<<24) | (b<<16) | (c<<8) | d );
    return true;
  }
  return false;
}

inline void handleArmSwitch() {
  if (currentState == STARWARS_PRE_GAME && armSwitch.rose()) {
     safePlay(SOUND_POWER_LIGHTSABER);
  }

  if (settings.sudden_death_mode) {
    if (armSwitch.rose()) { 
       if (currentState == STANDBY || currentState == PROP_IDLE) {
          if (!isBombPlanted()) {
             safePlay(SOUND_MENU_CANCEL);
             return;
          }
          suddenDeathActive = true;
          bombArmedTimestamp = millis();
          safePlay(SOUND_BOMB_PLANTED);
          setState(ARMED);
       }
    } else if (armSwitch.fell()) { 
       if (currentState == ARMED && suddenDeathActive) {
          setState(DISARMED);
       }
       if ((currentState == EXPLODED || currentState == DISARMED) && suddenDeathActive) {
          setState(STANDBY);
       }
    }
    return; 
  }

  if (armSwitch.rose()) {
    if (currentState == DISARMED || currentState == EXPLODED || currentState == PROP_IDLE || currentState == AWAIT_ARM_TOGGLE || currentState == PROP_DUD) {
      resetSpecialModes(); 
      setState(STANDBY);
    }
  } else if (armSwitch.fell()) {
    if (currentState == STANDBY) {
        lastPingTime = millis(); // Reset ping timer
        setState(PROP_IDLE);
    }
  }
}

inline void processArmingCode(const char* code) {
    bool ee = settings.easter_eggs_enabled;
    // ... check for special codes first ...
    if (ee && strcmp(code, "501") == 0) { setState(STARWARS_PRE_GAME); enteredCode[0] = '\0'; return; }
    if (ee && strcmp(code, "1138") == 0) { strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_THX); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "8675309") == 0) { strcpy(activeArmCode, code); stored_duration_ram = settings.bomb_duration_ms; settings.bomb_duration_ms = 222000; bombArmedTimestamp = millis(); safePlay(SOUND_JENNY); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "3141592") == 0) { strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_NERD); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "1984") == 0) { terminatorModeActive = true; strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_HASTA_2); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "7777777") == 0) { strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_JACKPOT); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "007") == 0) { bondModeActive = true; strcpy(activeArmCode, code); stored_duration_ram = settings.bomb_duration_ms; settings.bomb_duration_ms = 105000; bombArmedTimestamp = millis(); safePlay(SOUND_BOND_INTRO); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "12345") == 0) { centerPrintC("IDIOT LUGGAGE?", 1); safePlay(SOUND_SPACEBALLS); delay(2500); enteredCode[0] = '\0'; setState(PROP_IDLE); return; }
    if (ee && strcmp(code, "0451") == 0) { strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_SOM_BITCH); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "14085") == 0) { strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_MGS_ALERT); c4OnEnterArmed(); setState(ARMED); return; }
    if (ee && strcmp(code, "0000000") == 0) { centerPrintC("TOO EASY", 1); safePlay(SOUND_LAME); delay(1500); enteredCode[0] = '\0'; setState(PROP_IDLE); return; }
    if (ee && strcmp(code, "666666") == 0) { doomModeActive = true; strcpy(activeArmCode, code); bombArmedTimestamp = millis(); safePlay(SOUND_DOOM_SLAYER); setState(ARMED); return; }
    if (ee && strcmp(code, "5318008") == 0) { strcpy(activeArmCode, code); setState(EASTER_EGG_2); return; }
    if (strcmp(code, "999") == 0) { centerPrintC("SERVO TEST", 1); centerPrintC("ACTIVATED", 2); startShellEjectorSequence(); enteredCode[0] = '\0'; return; }
    if (strcmp(code, MASTER_CODE) == 0) { strcpy(activeArmCode, code); setState(EASTER_EGG); return; }

    // Standard Arming
    if ((int)strlen(code) == CODE_LENGTH) {
        // FIXED CODE CHECK
        if (settings.fixed_code_enabled) {
            if (strcmp(code, settings.fixed_code_val) != 0) {
                // Wrong code
                centerPrintC("INVALID CODE", 1);
                safePlay(SOUND_MENU_CANCEL);
                delay(1000);
                enteredCode[0] = '\0';
                setState(PROP_IDLE);
                return;
            }
        }

        strcpy(activeArmCode, code);
        bombArmedTimestamp = millis();
        safePlay(SOUND_BOMB_PLANTED);
        c4OnEnterArmed();
        setState(ARMED);
    } else {
        setState(PROP_IDLE);
    }
}

inline void handleKeypadInput(char key) {
  if (!key) return;

  if (currentState == STARWARS_PRE_GAME) {
     if (isdigit(key)) safePlay(random(SOUND_SWING_START, SOUND_SWING_END + 1));
     if (key == '#') {
        if (!isBombPlanted()) { centerPrintC("ERROR: MUST PLANT", 1); safePlay(SOUND_MENU_CANCEL); delay(2000); return; }
        strcpy(activeArmCode, MASTER_CODE); 
        stored_duration_ram = settings.bomb_duration_ms; settings.bomb_duration_ms = 350000; 
        starWarsModeActive = true; bombArmedTimestamp = millis(); safePlay(SOUND_STAR_WARS_THEME); c4OnEnterArmed(); setState(ARMED);
        return;
     }
     if (key == '*') { resetSpecialModes(); setState(PROP_IDLE); return; }
     return; 
  }

  if (currentState == PROP_IDLE) setState(ARMING);
  if (currentState == ARMED) setState(DISARMING_KEYPAD);
  displayNeedsUpdate = true;

  if (isdigit(key)) {
    if ((int)strlen(enteredCode) < CODE_LENGTH) {
      int len = strlen(enteredCode);
      enteredCode[len] = key;
      enteredCode[len + 1] = '\0';}
    
    if (currentState == DISARMING_KEYPAD) {
      bool matched = false;
      if (strcmp(enteredCode, activeArmCode) == 0) matched = true;
      else if (strcmp(enteredCode, MASTER_CODE) == 0) matched = true;
      else if (starWarsModeActive && strcmp(enteredCode, "501") == 0) matched = true;

      if (matched) setState(DISARMED);
      else if ((int)strlen(enteredCode) >= CODE_LENGTH) {
        uint32_t elapsed = millis() - bombArmedTimestamp;
        uint32_t total   = settings.bomb_duration_ms;
        uint32_t remaining = (total > elapsed) ? (total - elapsed) : 0;
        uint32_t penalty = remaining / 2;
        if (remaining > 3000) bombArmedTimestamp -= penalty;
        uint32_t newRemaining = (remaining > penalty) ? (remaining - penalty) : 0;
        c4OnTimeCut(newRemaining);
        menuCancel();
        setState(ARMED);
      }
    }

  } else if (key == '*') {
    if (millis() - lastStarPressTime < DOUBLE_TAP_TIMEOUT) {
      if (currentState == ARMING) setState(PROP_IDLE);
      if (currentState == DISARMING_KEYPAD) setState(ARMED);
      lastStarPressTime = 0;
    } else {
      enteredCode[0] = '\0';
      lastStarPressTime = millis();
    }
  } else if (key == '#') {
    if (currentState == ARMING) {
      if (!isBombPlanted()) {
         centerPrintC("ERROR: MUST PLANT", 1); centerPrintC("ON SITE FIRST!", 2);
         safePlay(SOUND_MENU_CANCEL); delay(2000); setState(PROP_IDLE);
         return; 
      }
      processArmingCode(enteredCode);
    }
  }
}

inline void handleDisarmButton() {
  if (currentState == ARMED || currentState == DISARMING_KEYPAD) {
    if (disarmButton.fell()) setState(DISARMING_MANUAL);
  }
  if (currentState == DISARMING_MANUAL && disarmButton.rose()) {
      safeStop(); 
      setState(ARMED);
  }
}

inline void handleRfid() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  uint8_t adminUID[] = {0xDE, 0xAD, 0xBE, 0xEF}; 
  if (UIDUtil::equals_len_bytes(4, adminUID, rfid.uid.uidByte, rfid.uid.size)) {
      safePlay(SOUND_MENU_CONFIRM);
      resetSpecialModes();
      setState(STANDBY); 
      rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      return;
  }

  // Find tag in database
  int foundIndex = -1;
  for (int i = 0; i < settings.num_rfid_uids; i++) {
    if (UIDUtil::equals_len_bytes(settings.rfid_uids[i].len,
                                  settings.rfid_uids[i].bytes,
                                  rfid.uid.uidByte, rfid.uid.size)) {
      foundIndex = i; break;
    }
  }

  if (foundIndex != -1) {
    uint8_t type = settings.rfid_uids[foundIndex].type;

    // CASE 1: DISARMING CARD (Type 0)
    if (type == 0) {
        if (currentState == ARMED || currentState == DISARMING_KEYPAD) {
           setState(DISARMING_RFID);
        }
    }
    // CASE 2: ARMING CARD (Type 1)
    else if (type == 1) {
        if (currentState == PROP_IDLE || currentState == ARMING) {
            if (!isBombPlanted()) {
                safePlay(SOUND_MENU_CANCEL);
            } else {
                safePlay(SOUND_KEY_PRESS);
                // Trigger Auto-Typing
                autoTypingActive = true;
                enteredCode[0] = '\0'; // Clear buffer
                lastAutoTypeTime = millis();
                
                // Determine target code
                if (settings.rfid_arming_mode == 0) {
                    // Fixed Code
                    if (settings.fixed_code_enabled) strcpy(autoTypingTarget, settings.fixed_code_val);
                    else strcpy(autoTypingTarget, "7355608");
                } else {
                    // Random Code
                    for(int k=0; k<7; k++) autoTypingTarget[k] = (char)random('0', '9'+1);
                    autoTypingTarget[7] = '\0';
                }
            }
        }
    }
  }
  else {
    safePlay(SOUND_INVALID_CARD);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

inline void handleBeepLogic() {
  static uint32_t lastCurveCalc = 0;
  static uint32_t cachedInterval = 1000;
  static uint32_t cachedBeepDuration = BEEP_TONE_DURATION_MS;

  if (millis() - lastCurveCalc > 100) {
      uint32_t elapsed = millis() - bombArmedTimestamp;
      if (elapsed > settings.bomb_duration_ms) elapsed = settings.bomb_duration_ms;
      float progress = (float)elapsed / (float)settings.bomb_duration_ms;
      float virtual_t = progress * 45.0f; 
      float bps = 1.05f * powf(1.039f, virtual_t);
      cachedInterval = (uint32_t)(1000.0f / bps);
      if (cachedInterval < 50) cachedInterval = 50;
      if (cachedInterval < (BEEP_TONE_DURATION_MS * 2)) cachedBeepDuration = cachedInterval / 2;
      else cachedBeepDuration = BEEP_TONE_DURATION_MS;
      if (cachedBeepDuration < 60) cachedBeepDuration = 60;
      lastCurveCalc = millis();
  }

  uint32_t delta = millis() - lastBeepTimestamp;
  if (delta >= cachedInterval) { lastBeepTimestamp = millis(); delta = 0; }

  static bool isBeeping = false;
  if (delta < cachedBeepDuration) {
     if (!isBeeping) { beepStart(BEEP_TONE_FREQ); ledIsOn = true; isBeeping = true; }
  } else {
     if (isBeeping) { beepStop(); ledIsOn = false; isBeeping = false; }
  }
}

inline void handleConfigMode(char key) {
  if (key) {
    displayNeedsUpdate = true;
    if (currentConfigState != MENU_VOLUME) safePlay(SOUND_KEY_PRESS);

    switch(currentConfigState) {
      case MENU_MAIN: {
        const int N = 11;
        if (key == '2') configMenuIndex = (configMenuIndex - 1 + N) % N;
        if (key == '8') configMenuIndex = (configMenuIndex + 1) % N;
        if (key == '#') {
          safePlay(SOUND_MENU_CONFIRM);
          configInputBuffer[0] = '\0';
          switch (configMenuIndex) {
            case 0: currentConfigState = MENU_SET_BOMB_TIME; break;
            case 1: currentConfigState = MENU_SET_MANUAL_TIME; break;
            case 2: currentConfigState = MENU_SET_RFID_TIME; break;
            case 3: currentConfigState = MENU_FIXED_CODE_SETTINGS; break; // NEW
            case 4: currentConfigState = MENU_SUDDEN_DEATH_TOGGLE; break;
            case 5: currentConfigState = MENU_DUD_SETTINGS; break;
            case 6: rfidViewIndex = 0; currentConfigState = MENU_VIEW_RFIDS; break;
            case 7: currentConfigState = MENU_HARDWARE_SUBMENU; break; 
            case 8: currentConfigState = MENU_NETWORK; break;
            case 9: { // SAVE EXIT
              Serial.println("[CFG] Save Exit");
              currentConfigState = MENU_SAVE_EXIT;
              displayNeedsUpdate = true;
              saveSettings();
              safePlay(SOUND_MENU_CONFIRM);
              delay(100); 
              requestRestart(600);
            } break;
            case 10: { // EXIT
              currentConfigState = MENU_EXIT_NO_SAVE;
              displayNeedsUpdate = true;
              safePlay(SOUND_MENU_CANCEL);
              delay(500); configInputBuffer[0]='\0'; setState(STANDBY);
            } break;
          }
        }
      } break;

      case MENU_SET_BOMB_TIME:
      case MENU_SET_MANUAL_TIME:
      case MENU_SET_RFID_TIME: {
        if (isdigit(key)) {
          size_t len = strlen(configInputBuffer);
          if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
          size_t len = strlen(configInputBuffer);
          if (len) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_MAIN;
        }
        if (key == '#') {
          unsigned long v = strtoul(configInputBuffer, nullptr, 10);
          if (v > 0) {
            if (currentConfigState == MENU_SET_BOMB_TIME)   settings.bomb_duration_ms = v;
            if (currentConfigState == MENU_SET_MANUAL_TIME) settings.manual_disarm_time_ms = v;
            if (currentConfigState == MENU_SET_RFID_TIME)   settings.rfid_disarm_time_ms = v;
            safePlay(SOUND_MENU_CONFIRM);
          } else safePlay(SOUND_MENU_CANCEL);
          currentConfigState = MENU_MAIN;
        }
      } break;

      // --- FIXED CODE ---
      case MENU_FIXED_CODE_SETTINGS: {
        if (key == '1') currentConfigState = MENU_FIXED_CODE_TOGGLE;
        if (key == '2') { configInputBuffer[0] = '\0'; currentConfigState = MENU_SET_FIXED_CODE; }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;
      case MENU_FIXED_CODE_TOGGLE: {
        if (key == '#') { settings.fixed_code_enabled = !settings.fixed_code_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_FIXED_CODE_SETTINGS;
      } break;
      case MENU_SET_FIXED_CODE: {
        if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len < 7) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0';
            else currentConfigState = MENU_FIXED_CODE_SETTINGS;
        }
        if (key == '#') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) {
               strcpy(settings.fixed_code_val, configInputBuffer);
               safePlay(SOUND_MENU_CONFIRM);
            } else safePlay(SOUND_MENU_CANCEL);
            currentConfigState = MENU_FIXED_CODE_SETTINGS;
        }
      } break;

      case MENU_SUDDEN_DEATH_TOGGLE: {
        if (key == '#') { settings.sudden_death_mode = !settings.sudden_death_mode; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_DUD_SETTINGS: {
        if (key == '#') { settings.dud_enabled = !settings.dud_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '1') { configInputBuffer[0]='\0'; currentConfigState = MENU_DUD_CHANCE; }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_DUD_CHANCE: {
        if (isdigit(key)) {
          size_t len = strlen(configInputBuffer);
          if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_DUD_SETTINGS;
        }
        if (key == '#') {
          int val = atoi(configInputBuffer);
          if (val >= 0 && val <= 100) { settings.dud_chance = val; safePlay(SOUND_MENU_CONFIRM); } 
          else safePlay(SOUND_MENU_CANCEL);
          currentConfigState = MENU_DUD_SETTINGS;
        }
      } break;

      case MENU_HARDWARE_SUBMENU: {
        if (key == '1') currentConfigState = MENU_AUDIO_SUBMENU;
        if (key == '2') currentConfigState = MENU_SERVO_SETTINGS;
        if (key == '3') currentConfigState = MENU_PLANT_SENSOR_TOGGLE;
        if (key == '4') currentConfigState = MENU_EXTRAS_SUBMENU;
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_AUDIO_SUBMENU: {
         if (key == '1') currentConfigState = MENU_AUDIO_TOGGLE;
         if (key == '2') { configInputBuffer[0]='\0'; currentConfigState = MENU_VOLUME; }
         if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;

      case MENU_AUDIO_TOGGLE: {
         if (key == '#') { settings.sound_enabled = !settings.sound_enabled; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_AUDIO_SUBMENU;
      } break;

      case MENU_VOLUME: {
         if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
         }
         if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_AUDIO_SUBMENU;
         }
         if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 30) { settings.sound_volume = val; safeVolume(val); }
            currentConfigState = MENU_AUDIO_SUBMENU;
         }
      } break;

      case MENU_PLANT_SENSOR_TOGGLE: {
         if (key == '#') { settings.plant_sensor_enabled = !settings.plant_sensor_enabled; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;

      case MENU_EXTRAS_SUBMENU: {
        if (key == '1') currentConfigState = MENU_TOGGLE_STROBE;
        if (key == '2') currentConfigState = MENU_TOGGLE_EASTER_EGGS;
        if (key == '3') currentConfigState = MENU_PING_SETTINGS;
        if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;
      
      // PING SETTINGS
      case MENU_PING_SETTINGS: {
        if (key == '1') currentConfigState = MENU_PING_TOGGLE;
        if (key == '2') { configInputBuffer[0]='\0'; currentConfigState = MENU_PING_INTERVAL; }
        if (key == '3') currentConfigState = MENU_PING_LIGHT_TOGGLE;
        if (key == '*') currentConfigState = MENU_EXTRAS_SUBMENU;
      } break;
      case MENU_PING_TOGGLE: {
        if (key == '#') { settings.ping_enabled = !settings.ping_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_PING_SETTINGS;
      } break;
      case MENU_PING_INTERVAL: {
        if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_PING_SETTINGS;
        }
        if (key == '#') {
           int val = atoi(configInputBuffer);
           if (val >= 5 && val <= 3600) { settings.ping_interval_s = val; safePlay(SOUND_MENU_CONFIRM); }
           else safePlay(SOUND_MENU_CANCEL);
           currentConfigState = MENU_PING_SETTINGS;
        }
      } break;
      case MENU_PING_LIGHT_TOGGLE: {
        if (key == '#') { settings.ping_light_enabled = !settings.ping_light_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_PING_SETTINGS;
      } break;

      case MENU_SERVO_SETTINGS: {
        if (key == '1') currentConfigState = MENU_SERVO_TOGGLE;
        if (key == '2') { configInputBuffer[0]='\0'; currentConfigState = MENU_SERVO_START_ANGLE; }
        if (key == '3') { configInputBuffer[0]='\0'; currentConfigState = MENU_SERVO_END_ANGLE; }
        if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;

      case MENU_SERVO_TOGGLE: {
        if (key == '#') { settings.servo_enabled = !settings.servo_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_SERVO_SETTINGS;
      } break;

      case MENU_SERVO_START_ANGLE: {
        if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_SERVO_SETTINGS;
        }
        if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 180) { settings.servo_start_angle = val; safePlay(SOUND_MENU_CONFIRM); } 
            else safePlay(SOUND_MENU_CANCEL);
            currentConfigState = MENU_SERVO_SETTINGS;
        }
      } break;

      case MENU_SERVO_END_ANGLE: {
        if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_SERVO_SETTINGS;
        }
        if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 180) { settings.servo_end_angle = val; safePlay(SOUND_MENU_CONFIRM); } 
            else safePlay(SOUND_MENU_CANCEL);
            currentConfigState = MENU_SERVO_SETTINGS;
        }
      } break;

      case MENU_TOGGLE_EASTER_EGGS: {
         if (key == '#') { settings.easter_eggs_enabled = !settings.easter_eggs_enabled; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_EXTRAS_SUBMENU;
      } break;

      case MENU_TOGGLE_STROBE: {
         if (key == '#') { settings.explosion_strobe_enabled = !settings.explosion_strobe_enabled; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_EXTRAS_SUBMENU;
      } break;

      case MENU_VIEW_RFIDS: {
        int total = settings.num_rfid_uids + 3;
        if (key == '2') rfidViewIndex = (rfidViewIndex - 1 + total) % total;
        if (key == '8') rfidViewIndex = (rfidViewIndex + 1) % total;
        if (key == '#') {
          if (rfidViewIndex < settings.num_rfid_uids) {
            // EDIT/DELETE EXISTING
            currentConfigState = MENU_DELETE_RFID_CONFIRM;
          } else if (rfidViewIndex == settings.num_rfid_uids) {
            if (settings.num_rfid_uids < MAX_RFID_UIDS) {
               currentConfigState = MENU_ADD_RFID_SELECT_TYPE; // Ask type first
            } else safePlay(SOUND_MENU_CANCEL);
          } else {
             if (rfidViewIndex == settings.num_rfid_uids + 1) currentConfigState = MENU_RFID_ADV_SETTINGS;
             else if (rfidViewIndex == settings.num_rfid_uids + 2) currentConfigState = MENU_CLEAR_RFIDS_CONFIRM; 
          }
        }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      // NEW: DELETE CONFIRMATION LOGIC
      case MENU_DELETE_RFID_CONFIRM: {
         if (key == '#') {
            // Perform Delete
            if (rfidViewIndex < settings.num_rfid_uids) {
                // Shift array left
                for (int i = rfidViewIndex; i < settings.num_rfid_uids - 1; i++) {
                    settings.rfid_uids[i] = settings.rfid_uids[i+1];
                }
                settings.num_rfid_uids--;
                safePlay(SOUND_MENU_CONFIRM);
                // Adjust index if we deleted the last item
                if (rfidViewIndex >= settings.num_rfid_uids && rfidViewIndex > 0) {
                    rfidViewIndex--;
                }
            }
            currentConfigState = MENU_VIEW_RFIDS;
         }
         if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;

      case MENU_ADD_RFID_SELECT_TYPE: {
        if (key == '1') { // Disarm
           configInputBuffer[0] = '0'; // abuse buffer to store type
           currentConfigState = MENU_ADD_RFID_WAIT;
           rfid.PCD_Init(); 
        }
        if (key == '2') { // Arm
           configInputBuffer[0] = '1';
           currentConfigState = MENU_ADD_RFID_WAIT;
           rfid.PCD_Init();
        }
        if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;
      
      case MENU_ADD_RFID_WAIT: {
         if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;

      case MENU_RFID_ADV_SETTINGS: {
        if (key == '1') currentConfigState = MENU_RFID_ARMING_MODE;
        if (key == '2') { configInputBuffer[0]='\0'; currentConfigState = MENU_RFID_ENTRY_SPEED; }
        if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;
      case MENU_RFID_ARMING_MODE: {
         if (key == '#') { settings.rfid_arming_mode = !settings.rfid_arming_mode; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_RFID_ADV_SETTINGS;
      } break;
      case MENU_RFID_ENTRY_SPEED: {
         if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
         }
         if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0'; else currentConfigState = MENU_RFID_ADV_SETTINGS;
         }
         if (key == '#') {
            int val = atoi(configInputBuffer);
            settings.rfid_entry_speed_ms = val; safePlay(SOUND_MENU_CONFIRM); 
            currentConfigState = MENU_RFID_ADV_SETTINGS;
         }
      } break;

      case MENU_CLEAR_RFIDS_CONFIRM: {
        if (key == '#') { settings.num_rfid_uids = 0; safePlay(SOUND_MENU_CONFIRM); currentConfigState = MENU_VIEW_RFIDS; }
        if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;

      case MENU_NETWORK: {
        if (key == '1')      currentConfigState = MENU_NET_ENABLE;
        else if (key == '2') currentConfigState = MENU_NET_SERVER_MODE;
        else if (key == '3') { currentConfigState = MENU_NET_IP;        configInputBuffer[0]='\0'; }
        else if (key == '4') { currentConfigState = MENU_NET_PORT;      configInputBuffer[0]='\0'; }
        else if (key == '9') { currentConfigState = MENU_NETWORK_2; } 
        else if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_NETWORK_2: {
        if (key == '5') { currentConfigState = MENU_NET_MASTER_IP; configInputBuffer[0]='\0'; }
        else if (key == '6') { currentConfigState = MENU_NET_WIFI_SETUP; startWiFiPortal(90); }
        else if (key == '7') { currentConfigState = MENU_NET_APPLY_NOW;  networkReconfigure(); }
        else if (key == '8') currentConfigState = MENU_NET_FORGET_CONFIRM; 
        else if (key == '9') currentConfigState = MENU_NETWORK; 
        else if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_NET_ENABLE: {
        if (key == '#') { settings.wifi_enabled = !settings.wifi_enabled; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_NETWORK;
      } break;

      case MENU_NET_SERVER_MODE: {
        if (key == '#') { settings.net_use_mdns = !settings.net_use_mdns; safePlay(SOUND_MENU_CONFIRM); }
        if (key == '*') currentConfigState = MENU_NETWORK;
      } break;

      case MENU_NET_IP:
      case MENU_NET_MASTER_IP: {
        if ((key >= '0' && key <= '9') || key == '.') {
          size_t len = strlen(configInputBuffer);
          if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
          size_t len = strlen(configInputBuffer);
          if (len) configInputBuffer[len-1] = '\0';
          else currentConfigState = MENU_NETWORK;
        }
        if (key == '#') {
          uint32_t ip;
          if (parseIpFromBuffer(configInputBuffer, ip)) {
            if (currentConfigState == MENU_NET_IP) settings.scoreboard_ip = ip;
            else settings.master_ip = ip;
            safePlay(SOUND_MENU_CONFIRM);
            currentConfigState = MENU_NETWORK;
          } else {
            safePlay(SOUND_MENU_CANCEL);
          }
        }
      } break;

      case MENU_NET_FORGET_CONFIRM: {
        if (key == '#') {
          forgetWifiCredentials(); 
          safePlay(SOUND_MENU_CONFIRM);
          currentConfigState = MENU_NETWORK_2; 
          displayNeedsUpdate = true;
        }
        if (key == '*') {
          currentConfigState = MENU_NETWORK_2; 
          displayNeedsUpdate = true;
        }
      } break;

      case MENU_NET_PORT: {
        if (isdigit(key)) {
          size_t len = strlen(configInputBuffer);
          if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        if (key == '*') {
          size_t len = strlen(configInputBuffer);
          if (len) configInputBuffer[len-1] = '\0';
          else currentConfigState = MENU_NETWORK;
        }
        if (key == '#') {
          unsigned long v = strtoul(configInputBuffer, nullptr, 10);
          if (v > 0 && v <= 65535) { settings.scoreboard_port = (uint16_t)v; safePlay(SOUND_MENU_CONFIRM); currentConfigState = MENU_NETWORK; }
          else safePlay(SOUND_MENU_CANCEL);
        }
      } break;

      case MENU_NET_WIFI_SETUP: {
        if (key == '*') { stopWiFiPortal(); currentConfigState = MENU_NETWORK_2; }
      } break;

      case MENU_NET_APPLY_NOW: {
        currentConfigState = MENU_NETWORK_2;
      } break;

      case MENU_SAVE_EXIT: { } break;
      case MENU_EXIT_NO_SAVE: { } break;

    }
  }

  // Handle Scan logic
  if (currentConfigState == MENU_ADD_RFID_WAIT) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      if (rfid.uid.size <= 10) {
        if (settings.num_rfid_uids < MAX_RFID_UIDS) {
          Settings::TagUID &slot = settings.rfid_uids[settings.num_rfid_uids];
          slot.len = rfid.uid.size;
          memcpy(slot.bytes, rfid.uid.uidByte, rfid.uid.size);
          // Set Type based on previous selection
          slot.type = (configInputBuffer[0] == '1') ? 1 : 0;
          
          settings.num_rfid_uids++;
          safePlay(SOUND_MENU_CONFIRM);
          String uid = String("Added: ") + UIDUtil::toHex(slot.bytes, slot.len);
          centerPrint(uid, 1);
          centerPrintC(slot.type==1 ? "[ARMING]" : "[DISARM]", 2);
          delay(1000);
          currentConfigState = MENU_VIEW_RFIDS;
        } else safePlay(SOUND_MENU_CANCEL);
      } else safePlay(SOUND_MENU_CANCEL);
      rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      displayNeedsUpdate = true;
    }
  }
}

inline void serviceGameplay(char key) {
  // PING LOGIC (Only in Idle)
  if (currentState == PROP_IDLE && settings.ping_enabled) {
      if (millis() - lastPingTime > (settings.ping_interval_s * 1000UL)) {
         safePlay(SOUND_PING);
         lastPingTime = millis();
      }
  }

  // FIX: AUTO TYPING ABORT (Moved Here)
  // If a real key is pressed while auto typing, we abort.
  if (key && autoTypingActive) {
      autoTypingActive = false;
      // We also process the key normally below, so it interrupts AND types the new number.
  }

  // AUTO TYPING LOGIC
  if (autoTypingActive) {
      if (millis() - lastAutoTypeTime > settings.rfid_entry_speed_ms) {
          lastAutoTypeTime = millis();
          int currentLen = strlen(enteredCode);
          int targetLen = strlen(autoTypingTarget);
          
          if (currentLen < targetLen) {
              // Type next char
              handleKeypadInput(autoTypingTarget[currentLen]);
          } else {
              // Press Enter
              handleKeypadInput('#');
              autoTypingActive = false;
          }
      }
      return; // Skip normal key handling while auto-typing
  }

  switch (currentState) {
    case STARWARS_PRE_GAME:
    case PROP_IDLE:
    case ARMING:
      handleKeypadInput(key);
      handleRfid(); // Check for Arming cards
      break;

    case DISARMING_KEYPAD:
      handleKeypadInput(key);
      handleDisarmButton();
      handleRfid();
      handleBeepLogic();
      break;

    case ARMED:
      handleKeypadInput(key);
      handleDisarmButton();
      handleRfid();
      handleBeepLogic();
      break;

    case DISARMING_MANUAL:
      handleDisarmButton();
      handleBeepLogic();
      if (millis() - disarmStartTimestamp >= settings.manual_disarm_time_ms)
        setState(DISARMED);
      break;

    case DISARMING_RFID:
      handleBeepLogic();
      if (millis() - disarmStartTimestamp >= settings.rfid_disarm_time_ms)
        setState(DISARMED);
      break;

    case PRE_EXPLOSION:
      if (settings.servo_enabled && !servoTriggeredThisExplosion) {
         if (millis() - stateEntryTimestamp >= 4500) {
             startShellEjectorSequence();
             servoTriggeredThisExplosion = true;
         }
      }
      break;

    default:
      break;
  }
}