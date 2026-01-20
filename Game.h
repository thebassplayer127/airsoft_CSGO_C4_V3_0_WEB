// Game.h
// VERSION: 6.2.0
// OPTIMIZED: Beep curve calculation throttled to 100ms
// FIXED: Suppressed audio in Volume Menu to prevent DFPlayer freeze
// FIXED: Allow "501" to disarm in Star Wars mode
// FIXED: Added OTA update support (via Network.h integration)

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

// Cache for unsaved RAM settings (so we don't revert to EEPROM after special modes)
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
  // --- STAR WARS MODE TOGGLE FX (Pre-Game) ---
  if (currentState == STARWARS_PRE_GAME && armSwitch.rose()) {
     safePlay(SOUND_POWER_LIGHTSABER);
  }

  // --- SUDDEN DEATH MODE LOGIC ---
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

  // --- NORMAL LOGIC ---
  if (armSwitch.rose()) {
    if (currentState == DISARMED || currentState == EXPLODED || currentState == PROP_IDLE || currentState == AWAIT_ARM_TOGGLE || currentState == PROP_DUD) {
      resetSpecialModes(); 
      setState(STANDBY);
    }
  } else if (armSwitch.fell()) {
    if (currentState == STANDBY) setState(PROP_IDLE);
  }
}

inline void handleKeypadInput(char key) {
  if (!key) return;

  // --- STAR WARS PRE-GAME FX ---
  if (currentState == STARWARS_PRE_GAME) {
     if (isdigit(key)) {
        safePlay(random(SOUND_SWING_START, SOUND_SWING_END + 1));
     }
     if (key == '#') {
        if (!isBombPlanted()) {
           centerPrintC("ERROR: MUST PLANT", 1);
           safePlay(SOUND_MENU_CANCEL);
           delay(2000); 
           return; 
        }
        strcpy(activeArmCode, MASTER_CODE); 
        
        // Cache current RAM duration before override
        stored_duration_ram = settings.bomb_duration_ms;
        settings.bomb_duration_ms = 350000; 

        starWarsModeActive = true;
        bombArmedTimestamp = millis();
        safePlay(SOUND_STAR_WARS_THEME); 
        c4OnEnterArmed(); 
        setState(ARMED);
        return;
     }
     if (key == '*') {
        resetSpecialModes();
        setState(PROP_IDLE);
        return;
     }
     return; 
  }

  if (currentState == PROP_IDLE) setState(ARMING);
  if (currentState == ARMED) setState(DISARMING_KEYPAD);
  displayNeedsUpdate = true;

  if (isdigit(key)) {
    if ((int)strlen(enteredCode) < CODE_LENGTH) {
      int len = strlen(enteredCode);
      enteredCode[len] = key;
      enteredCode[len + 1] = '\0';
    }
    
    // --- Disarm Check Logic ---
    if (currentState == DISARMING_KEYPAD) {
      bool matched = false;
      if (strcmp(enteredCode, activeArmCode) == 0) matched = true;
      else if (strcmp(enteredCode, MASTER_CODE) == 0) matched = true;
      // FIX: Star Wars Exception - allow 501 to disarm even though it's short
      else if (starWarsModeActive && strcmp(enteredCode, "501") == 0) matched = true;

      if (matched) {
        setState(DISARMED);
      } 
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
         centerPrintC("ERROR: MUST PLANT", 1);
         centerPrintC("ON SITE FIRST!", 2);
         safePlay(SOUND_MENU_CANCEL);
         delay(2000); 
         setState(PROP_IDLE);
         return; 
      }

      resetSpecialModes();
      bool ee = settings.easter_eggs_enabled;

      // --- CHECK CODES ---
      if (ee && strcmp(enteredCode, "501") == 0) {
         setState(STARWARS_PRE_GAME);
         enteredCode[0] = '\0'; 
      } else if (ee && strcmp(enteredCode, "1138") == 0) { 
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_THX); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "8675309") == 0) {
         strcpy(activeArmCode, enteredCode);
         
         // Cache current RAM duration before override
         stored_duration_ram = settings.bomb_duration_ms;
         settings.bomb_duration_ms = 222000;

         bombArmedTimestamp = millis();
         safePlay(SOUND_JENNY); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "3141592") == 0) {
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_NERD); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "1984") == 0) {
         terminatorModeActive = true;
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_HASTA_2); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "7777777") == 0) {
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_JACKPOT); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "007") == 0) {
         bondModeActive = true;
         strcpy(activeArmCode, enteredCode);

         // Cache current RAM duration before override
         stored_duration_ram = settings.bomb_duration_ms;
         settings.bomb_duration_ms = 105000;

         bombArmedTimestamp = millis();
         safePlay(SOUND_BOND_INTRO); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "12345") == 0) {
         centerPrintC("IDIOT LUGGAGE?", 1);
         safePlay(SOUND_SPACEBALLS);
         delay(2500);
         enteredCode[0] = '\0';
         setState(PROP_IDLE);
      } else if (ee && strcmp(enteredCode, "0451") == 0) {
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_SOM_BITCH); 
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "14085") == 0) {
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_MGS_ALERT);
         c4OnEnterArmed(); setState(ARMED);
      } else if (ee && strcmp(enteredCode, "0000000") == 0) {
         centerPrintC("TOO EASY", 1);
         safePlay(SOUND_LAME);
         delay(1500);
         enteredCode[0] = '\0';
         setState(PROP_IDLE);
      } else if (ee && strcmp(enteredCode, "666666") == 0) { 
         doomModeActive = true;
         strcpy(activeArmCode, enteredCode);
         bombArmedTimestamp = millis();
         safePlay(SOUND_DOOM_SLAYER); 
         setState(ARMED);
      } else if (ee && strcmp(enteredCode, "5318008") == 0) {
          strcpy(activeArmCode, enteredCode);
          setState(EASTER_EGG_2);

      } else if (strcmp(enteredCode, "999") == 0) {
          centerPrintC("SERVO TEST", 1);
          centerPrintC("ACTIVATED", 2);
          Serial.println("[GAME] Manual Servo Test Triggered via 999#");
          startShellEjectorSequence();
          enteredCode[0] = '\0';
          return;

      } else if (strcmp(enteredCode, MASTER_CODE) == 0) {
          strcpy(activeArmCode, enteredCode);
          setState(EASTER_EGG); 

      } else if ((int)strlen(enteredCode) == CODE_LENGTH) {
        strcpy(activeArmCode, enteredCode);
        bombArmedTimestamp = millis();
        safePlay(SOUND_BOMB_PLANTED);
        c4OnEnterArmed();
        setState(ARMED);
        
      } else {
        setState(PROP_IDLE);
      }
    }
  }
}

inline void handleDisarmButton() {
  if (currentState == ARMED || currentState == DISARMING_KEYPAD) {
    if (disarmButton.fell()) setState(DISARMING_MANUAL);
  }
  // FIX: Explicitly stop audio when releasing button to cut off Loop track
  if (currentState == DISARMING_MANUAL && disarmButton.rose()) {
      myDFPlayer.stop(); 
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
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return;
  }

  bool isAuthorized = false;
  for (int i = 0; i < settings.num_rfid_uids; i++) {
    if (UIDUtil::equals_len_bytes(settings.rfid_uids[i].len,
                                  settings.rfid_uids[i].bytes,
                                  rfid.uid.uidByte, rfid.uid.size)) {
      isAuthorized = true; break;
    }
  }

  if (isAuthorized && (currentState == ARMED || currentState == DISARMING_KEYPAD)) {
    setState(DISARMING_RFID);
  }
  else if (!isAuthorized) {
    safePlay(SOUND_INVALID_CARD);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

inline void handleBeepLogic() {
  // --- Robust Beep State Machine ---
  
  // Optimization: Recalculate curve only every 100ms
  static uint32_t lastCurveCalc = 0;
  static uint32_t cachedInterval = 1000;
  static uint32_t cachedBeepDuration = BEEP_TONE_DURATION_MS;

  if (millis() - lastCurveCalc > 100) {
      uint32_t elapsed = millis() - bombArmedTimestamp;
      if (elapsed > settings.bomb_duration_ms) elapsed = settings.bomb_duration_ms;

      float progress = (float)elapsed / (float)settings.bomb_duration_ms;
      float virtual_t = progress * 45.0f; 

      // CS:GO Curve
      float bps = 1.05f * powf(1.039f, virtual_t);
      cachedInterval = (uint32_t)(1000.0f / bps);

      // Determine Duration
      if (cachedInterval < (BEEP_TONE_DURATION_MS * 2)) {
         cachedBeepDuration = cachedInterval / 2;
      } else {
         cachedBeepDuration = BEEP_TONE_DURATION_MS;
      }
      lastCurveCalc = millis();
  }

  // --- Cycle Calculation ---
  uint32_t delta = millis() - lastBeepTimestamp;
  if (delta >= cachedInterval) {
     lastBeepTimestamp = millis();
     delta = 0;
  }

  // Robust ON/OFF Logic
  static bool isBeeping = false;
  
  if (delta < cachedBeepDuration) {
     if (!isBeeping) {
        beepStart(BEEP_TONE_FREQ);
        ledIsOn = true;
        isBeeping = true;
     }
  } else {
     if (isBeeping) {
        beepStop();
        ledIsOn = false;
        isBeeping = false;
     }
  }
}

inline void handleConfigMode(char key) {
  if (key) {
    displayNeedsUpdate = true;
    
    // FIX: Suppress keypress sounds specifically in Volume menu to prevent DFPlayer freeze
    if (currentConfigState != MENU_VOLUME) {
        safePlay(SOUND_KEY_PRESS); 
    }

    switch(currentConfigState) {
      case MENU_MAIN: {
        const int N = 10; // Increased to 10 items
        if (key == '2') configMenuIndex = (configMenuIndex - 1 + N) % N;
        if (key == '8') configMenuIndex = (configMenuIndex + 1) % N;
        if (key == '#') {
          safePlay(SOUND_MENU_CONFIRM);
          configInputBuffer[0] = '\0';
          switch (configMenuIndex) {
            case 0: currentConfigState = MENU_SET_BOMB_TIME; break;
            case 1: currentConfigState = MENU_SET_MANUAL_TIME; break;
            case 2: currentConfigState = MENU_SET_RFID_TIME; break;
            case 3: currentConfigState = MENU_SUDDEN_DEATH_TOGGLE; break;
            case 4: currentConfigState = MENU_DUD_SETTINGS; break;
            case 5: rfidViewIndex = 0; currentConfigState = MENU_VIEW_RFIDS; break;
            case 6: currentConfigState = MENU_HARDWARE_SUBMENU; break; 
            case 7: currentConfigState = MENU_NETWORK; break;
            case 8: {
              Serial.println("[CFG] Save Exit");
              currentConfigState = MENU_SAVE_EXIT;
              displayNeedsUpdate = true;
              saveSettings();
              safePlay(SOUND_MENU_CONFIRM);
              delay(100); 
              requestRestart(600);
            } break;
            case 9: {
              // Exit without saving changes to EEPROM, BUT KEEP RAM
              currentConfigState = MENU_EXIT_NO_SAVE;
              displayNeedsUpdate = true;
              safePlay(SOUND_MENU_CANCEL);
              delay(500); 
              configInputBuffer[0]='\0';
              setState(STANDBY);
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
          if (len) configInputBuffer[len-1] = '\0';
          else currentConfigState = MENU_MAIN;
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
        // FIX: Added Backspace Logic
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0';
            else currentConfigState = MENU_DUD_SETTINGS;
        }
        if (key == '#') {
          int val = atoi(configInputBuffer);
          if (val >= 0 && val <= 100) { 
            settings.dud_chance = val; 
            safePlay(SOUND_MENU_CONFIRM); 
          } else {
            safePlay(SOUND_MENU_CANCEL);
          }
          currentConfigState = MENU_DUD_SETTINGS;
        }
      } break;

      // --- HARDWARE / AUDIO SUBMENU ---
      case MENU_HARDWARE_SUBMENU: {
        if (key == '1') currentConfigState = MENU_AUDIO_SUBMENU;
        if (key == '2') currentConfigState = MENU_SERVO_SETTINGS;
        if (key == '3') currentConfigState = MENU_PLANT_SENSOR_TOGGLE;
        if (key == '4') currentConfigState = MENU_EXTRAS_SUBMENU; // FX
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      // --- AUDIO SUBMENU ---
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
         // FIX: Added Backspace Logic
         if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0';
            else currentConfigState = MENU_AUDIO_SUBMENU;
         }
         if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 30) { 
                settings.sound_volume = val; 
                // FIX: Removed delay and confirmation sound to prevent lockup
                myDFPlayer.volume(val); 
            } else {
                // Not playing cancel sound either, to be safe.
            }
            currentConfigState = MENU_AUDIO_SUBMENU;
         }
      } break;

      case MENU_PLANT_SENSOR_TOGGLE: {
         if (key == '#') { settings.plant_sensor_enabled = !settings.plant_sensor_enabled; safePlay(SOUND_MENU_CONFIRM); }
         if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;

      // --- EXTRAS (FX) SUBMENU ---
      case MENU_EXTRAS_SUBMENU: {
        if (key == '1') currentConfigState = MENU_TOGGLE_STROBE;
        if (key == '2') currentConfigState = MENU_TOGGLE_EASTER_EGGS;
        if (key == '*') currentConfigState = MENU_HARDWARE_SUBMENU;
      } break;

      // --- SERVO SETTINGS ---
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
        // FIX: Added Backspace Logic
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0';
            else currentConfigState = MENU_SERVO_SETTINGS;
        }
        if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 180) { 
              settings.servo_start_angle = val; 
              safePlay(SOUND_MENU_CONFIRM); 
            } else {
              safePlay(SOUND_MENU_CANCEL);
            }
            currentConfigState = MENU_SERVO_SETTINGS;
        }
      } break;

      case MENU_SERVO_END_ANGLE: {
        if (isdigit(key)) {
            size_t len = strlen(configInputBuffer);
            if (len + 1 < CONFIG_INPUT_MAX) { configInputBuffer[len] = key; configInputBuffer[len+1] = '\0'; }
        }
        // FIX: Added Backspace Logic
        if (key == '*') {
            size_t len = strlen(configInputBuffer);
            if (len > 0) configInputBuffer[len-1] = '\0';
            else currentConfigState = MENU_SERVO_SETTINGS;
        }
        if (key == '#') {
            int val = atoi(configInputBuffer);
            if (val >= 0 && val <= 180) { 
              settings.servo_end_angle = val; 
              safePlay(SOUND_MENU_CONFIRM); 
            } else {
              safePlay(SOUND_MENU_CANCEL);
            }
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
        int total = settings.num_rfid_uids + 2;
        if (key == '2') rfidViewIndex = (rfidViewIndex - 1 + total) % total;
        if (key == '8') rfidViewIndex = (rfidViewIndex + 1) % total;
        if (key == '#') {
          if (rfidViewIndex < settings.num_rfid_uids) {
          } else if (rfidViewIndex == settings.num_rfid_uids) {
            if (settings.num_rfid_uids < MAX_RFID_UIDS) {
               currentConfigState = MENU_ADD_RFID;
               // Wake up reader when entering Add Mode
               rfid.PCD_Init(); 
            }
            else safePlay(SOUND_MENU_CANCEL);
          } else {
            currentConfigState = MENU_CLEAR_RFIDS_CONFIRM;
          }
        }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_ADD_RFID: { if (key == '*') currentConfigState = MENU_VIEW_RFIDS; } break;

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

  if (currentConfigState == MENU_ADD_RFID) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      if (rfid.uid.size <= 10) {
        if (settings.num_rfid_uids < MAX_RFID_UIDS) {
          Settings::TagUID &slot = settings.rfid_uids[settings.num_rfid_uids];
          slot.len = rfid.uid.size;
          memcpy(slot.bytes, rfid.uid.uidByte, rfid.uid.size);
          settings.num_rfid_uids++;
          safePlay(SOUND_MENU_CONFIRM);
          String uid = String("Added: ") + UIDUtil::toHex(slot.bytes, slot.len);
          centerPrint(uid, 1);
          delay(600);
          currentConfigState = MENU_VIEW_RFIDS;
        } else safePlay(SOUND_MENU_CANCEL);
      } else safePlay(SOUND_MENU_CANCEL);
      rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      displayNeedsUpdate = true;
    }
  }
}

inline void serviceGameplay(char key) {
  switch (currentState) {
    case STARWARS_PRE_GAME:
    case PROP_IDLE:
    case ARMING:
      handleKeypadInput(key);
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

    // Trigger Servo/Strobe check inside gameplay loop when exploded/pre-exploded
    case PRE_EXPLOSION:
      // Timing Check: 4500ms after entry (sync with strobe)
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