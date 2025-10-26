// Game.h
//VERSION: 3.0.0
// 10.26.2025

#pragma once
#include <Arduino.h>
#include "State.h"
#include "Hardware.h"
#include "Display.h"
#include "Utils.h"
#include "Network.h"
#include "C4Net.h"

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
  if (armSwitch.rose()) {
    if (currentState == DISARMED || currentState == EXPLODED || currentState == PROP_IDLE || currentState == AWAIT_ARM_TOGGLE)
      setState(STANDBY);
  } else if (armSwitch.fell()) {
    if (currentState == STANDBY) setState(PROP_IDLE);
  }
}

inline void handleKeypadInput(char key) {
  if (!key) return;
  if (currentState == PROP_IDLE) setState(ARMING);
  if (currentState == ARMED) setState(DISARMING_KEYPAD);
  displayNeedsUpdate = true;

  if (isdigit(key)) {
    if ((int)strlen(enteredCode) < CODE_LENGTH) {
      int len = strlen(enteredCode);
      enteredCode[len] = key;
      enteredCode[len + 1] = '\0';
    }
    if (currentState == DISARMING_KEYPAD && (int)strlen(enteredCode) == CODE_LENGTH) {
      if (strcmp(enteredCode, activeArmCode) == 0 || strcmp(enteredCode, MASTER_CODE) == 0) {
        setState(DISARMED);
      } else {
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
      if ((int)strlen(enteredCode) == CODE_LENGTH) {
        strcpy(activeArmCode, enteredCode);

        if (strcmp(enteredCode, "5318008") == 0) {
          setState(EASTER_EGG_2);

        } else if (strcmp(enteredCode, MASTER_CODE) == 0) {
          setState(EASTER_EGG);

        } else {
        
          bombArmedTimestamp = millis();
          myDFPlayer.play(SOUND_BOMB_PLANTED);
          c4OnEnterArmed();                 // ⬅ tell scoreboard: planted + duration
          setState(ARMED);
        }
        
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
  if (currentState == DISARMING_MANUAL && disarmButton.rose()) setState(ARMED);
}

inline void handleRfid() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  bool isAuthorized = false;
  for (int i = 0; i < settings.num_rfid_uids; i++) {
    if (UIDUtil::equals_len_bytes(settings.rfid_uids[i].len,
                                  settings.rfid_uids[i].bytes,
                                  rfid.uid.uidByte, rfid.uid.size)) {
      isAuthorized = true; break;
    }
  }
  if (isAuthorized && currentState == ARMED) setState(DISARMING_RFID);
  else if (!isAuthorized) myDFPlayer.play(SOUND_INVALID_CARD);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Countdown/tension beep — call this in ARMED and all disarm states
inline void handleBeepLogic() {
  float t   = (millis() - bombArmedTimestamp) / 1000.0f;
  float bps = 1.05f * powf(1.039f, t);
  uint32_t interval = (uint32_t)(1000.0f / bps);

  if (millis() - lastBeepTimestamp >= interval) {
    lastBeepTimestamp = millis();
    beepStart(BEEP_TONE_FREQ);
    ledIsOn = true;
  } else {
    if (millis() - lastBeepTimestamp > BEEP_TONE_DURATION_MS) {
      beepStop();
      ledIsOn = false;
    }
  }
}

// ----------- MENU / CONFIG -----------

inline void handleConfigMode(char key) {
  if (key) {
    displayNeedsUpdate = true;
    menuClick();   // non-blocking menu beep

    switch(currentConfigState) {
      case MENU_MAIN: {
        const int N = 6;
        if (key == '2') configMenuIndex = (configMenuIndex - 1 + N) % N;
        if (key == '8') configMenuIndex = (configMenuIndex + 1) % N;
        if (key == '#') {
          menuConfirm();
          configInputBuffer[0] = '\0';
          switch (configMenuIndex) {
            case 0: currentConfigState = MENU_SET_BOMB_TIME; break;
            case 1: currentConfigState = MENU_SET_MANUAL_TIME; break;
            case 2: currentConfigState = MENU_SET_RFID_TIME; break;
            case 3: rfidViewIndex = 0; currentConfigState = MENU_VIEW_RFIDS; break;
            case 4: currentConfigState = MENU_NETWORK; break;
            case 5: {
  // Immediately show the screen, persist, and schedule reboot.
  Serial.println("[CFG] Entering MENU_SAVE_EXIT screen");
  currentConfigState = MENU_SAVE_EXIT;
  displayNeedsUpdate = true;

  bool ok = saveSettings();
  Serial.printf("[CFG] saveSettings() returned %s\n", ok ? "true" : "false");

  menuConfirm();                 // short beep; non-blocking
  requestRestart(600);           // restartPump() will handle reboot
} break;

          }
        }
      } break;

      // Time entries
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
            menuConfirm();
          } else menuCancel();
          currentConfigState = MENU_MAIN;
        }
      } break;

      // RFID list
      case MENU_VIEW_RFIDS: {
        int total = settings.num_rfid_uids + 2;
        if (key == '2') rfidViewIndex = (rfidViewIndex - 1 + total) % total;
        if (key == '8') rfidViewIndex = (rfidViewIndex + 1) % total;
        if (key == '#') {
          if (rfidViewIndex < settings.num_rfid_uids) {
            // reserved (e.g., delete/replace)
          } else if (rfidViewIndex == settings.num_rfid_uids) {
            if (settings.num_rfid_uids < MAX_RFID_UIDS) currentConfigState = MENU_ADD_RFID;
            else menuCancel();
          } else {
            currentConfigState = MENU_CLEAR_RFIDS_CONFIRM;
          }
        }
        if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_ADD_RFID: { if (key == '*') currentConfigState = MENU_VIEW_RFIDS; } break;

      case MENU_CLEAR_RFIDS_CONFIRM: {
        if (key == '#') { settings.num_rfid_uids = 0; menuConfirm(); currentConfigState = MENU_VIEW_RFIDS; }
        if (key == '*') currentConfigState = MENU_VIEW_RFIDS;
      } break;

      // Network submenu
      case MENU_NETWORK: {
        if (key == '1')      currentConfigState = MENU_NET_ENABLE;
        else if (key == '2') currentConfigState = MENU_NET_SERVER_MODE;
        else if (key == '3') { currentConfigState = MENU_NET_IP;        configInputBuffer[0]='\0'; }
        else if (key == '4') { currentConfigState = MENU_NET_PORT;      configInputBuffer[0]='\0'; }
        else if (key == '5') { currentConfigState = MENU_NET_MASTER_IP; configInputBuffer[0]='\0'; }
        else if (key == '6') { currentConfigState = MENU_NET_WIFI_SETUP; startWiFiPortal(90); }
        else if (key == '7') { currentConfigState = MENU_NET_APPLY_NOW;  networkReconfigure(); }
        else if (key == '9') { currentConfigState = MENU_NETWORK_2; }
        else if (key == '*') currentConfigState = MENU_MAIN;
      } break;

      case MENU_NETWORK_2: {
  if (key == '9') currentConfigState = MENU_NETWORK;
  else if (key == '8') currentConfigState = MENU_NET_FORGET_CONFIRM;   // <<< NEW
  else if (key == '*') currentConfigState = MENU_MAIN;
} break;

      case MENU_NET_ENABLE: {
        if (key == '#') { settings.wifi_enabled = !settings.wifi_enabled; menuConfirm(); }
        if (key == '*') currentConfigState = MENU_NETWORK;
      } break;

      case MENU_NET_SERVER_MODE: {
        if (key == '#') { settings.net_use_mdns = !settings.net_use_mdns; menuConfirm(); }
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
            menuConfirm();
            currentConfigState = MENU_NETWORK;
          } else {
            menuCancel();
          }
        }
      } break;

case MENU_NET_FORGET_CONFIRM: {
  if (key == '#') {
    forgetWifiCredentials();   // from Network.h
    menuConfirm();
    currentConfigState = MENU_NETWORK;   // return to first network page
    displayNeedsUpdate = true;
  }
  if (key == '*') {
    currentConfigState = MENU_NETWORK_2; // back to page 2 without changes
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
          if (v > 0 && v <= 65535) { settings.scoreboard_port = (uint16_t)v; menuConfirm(); currentConfigState = MENU_NETWORK; }
          else menuCancel();
        }
      } break;

      case MENU_NET_WIFI_SETUP: {
        // Non-blocking portal runs in networkLoop()
        if (key == '*') { stopWiFiPortal(); currentConfigState = MENU_NETWORK; }
      } break;

      case MENU_NET_APPLY_NOW: {
        // brief status; return automatically
        currentConfigState = MENU_NETWORK;
      } break;

      case MENU_SAVE_EXIT: { /* no-op: screen is drawn by Display.h while we wait */ } break;

    }
  }

  // Continuous actions in specific config states
  if (currentConfigState == MENU_ADD_RFID) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      if (rfid.uid.size <= 10) {
        if (settings.num_rfid_uids < MAX_RFID_UIDS) {
          Settings::TagUID &slot = settings.rfid_uids[settings.num_rfid_uids];
          slot.len = rfid.uid.size;
          memcpy(slot.bytes, rfid.uid.uidByte, rfid.uid.size);
          settings.num_rfid_uids++;
          menuConfirm();
          String uid = String("Added: ") + UIDUtil::toHex(slot.bytes, slot.len);
          centerPrint(uid, 1);
          delay(600);
          currentConfigState = MENU_VIEW_RFIDS;
        } else menuCancel();
      } else menuCancel();
      rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      displayNeedsUpdate = true;
    }
  }
}

// ----------- Tiny helper to call from .ino -----------
// Keeps gameplay logic (including continuous beep) running per-state.
inline void serviceGameplay(char key) {
  switch (currentState) {
    case PROP_IDLE:
    case ARMING:
      handleKeypadInput(key);
      break;

    case DISARMING_KEYPAD:
      handleKeypadInput(key);
      handleDisarmButton();
      handleRfid();
      handleBeepLogic();   // keep tension beep
      break;

    case ARMED:
      handleKeypadInput(key);
      handleDisarmButton();
      handleRfid();
      handleBeepLogic();   // keep tension beep
      break;

    case DISARMING_MANUAL:
      handleDisarmButton();
      handleBeepLogic();   // keep tension beep
      if (millis() - disarmStartTimestamp >= settings.manual_disarm_time_ms)
        setState(DISARMED);
      break;

    case DISARMING_RFID:
      handleBeepLogic();   // keep tension beep
      if (millis() - disarmStartTimestamp >= settings.rfid_disarm_time_ms)
        setState(DISARMED);
      break;

    default:
      break;
  }
}
