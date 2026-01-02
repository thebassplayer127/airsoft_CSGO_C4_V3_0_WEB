// Display.h
// VERSION: 5.3.0
// FIXED: Clean header definitions
// FIXED: Strobe Delay (4500ms)

#pragma once
#include "State.h"
#include "Hardware.h"
#include "Utils.h"
#include "ShellEjector.h"

// --- Helper Functions ---

inline void clearRow(int row) {
  lcd.setCursor(0,row); 
  lcd.print("                    ");
}

inline void centerPrint(const String& text, int row) {
  int textLength = text.length();
  int padding = (20 - textLength) / 2; 
  if (padding < 0) padding = 0;
  clearRow(row);
  lcd.setCursor(padding, row);
  lcd.print(text);
}

inline void centerPrintC(const char* text, int row) {
  int textLength = strlen(text);
  int padding = (20 - textLength) / 2; 
  if (padding < 0) padding = 0;
  clearRow(row);
  lcd.setCursor(padding, row);
  lcd.print(text);
}

inline String boolToOnOff(uint8_t v){ return v?String("ON"):String("OFF"); }
inline String modeToStr(uint8_t v){ return v?String("mDNS"):String("StaticIP"); }

// --- Main Display Logic ---

inline void updateDisplay() {
  if (currentState == CONFIG_MODE) {
    if (!displayNeedsUpdate) return;
    displayNeedsUpdate = false;
    lcd.clear(); yield();

    switch (currentConfigState) {
      case MENU_MAIN: {
        String header = String("CONFIG v") + FW_VERSION;
        centerPrint(header, 0);

        const char* items[] = {
          "Bomb Time", "Manual Disarm", "RFID Disarm",
          "Sudden Death", "Dud Settings", 
          "RFID Tags", "Settings & Servo", "Network",
          "Save & Exit", "Exit (No Save)"
        };
        const int TOTAL = 10; 

        for (int i = -1; i <= 1; ++i) {
          int row = i + 2;
          int itemIndex = (configMenuIndex + i + TOTAL) % TOTAL;
          clearRow(row);
          lcd.setCursor(0, row);
          String line = (i == 0 ? "> " : "  ");
          line += items[itemIndex];
          lcd.print(line);
        }
      } break;

      case MENU_SET_BOMB_TIME:
      case MENU_SET_MANUAL_TIME:
      case MENU_SET_RFID_TIME: {
        centerPrintC(
          (currentConfigState == MENU_SET_BOMB_TIME)   ? "Set Bomb Time (ms)" :
          (currentConfigState == MENU_SET_MANUAL_TIME) ? "Set Manual Time (ms)" :
                                                         "Set RFID Time (ms)", 0);
        char buffer[21]; snprintf(buffer, sizeof(buffer), "New: %s", configInputBuffer);
        centerPrintC(buffer, 2);
        centerPrintC("(#=Save, *=Back)", 3);
      } break;

      case MENU_SUDDEN_DEATH_TOGGLE: {
        centerPrintC("Sudden Death Mode", 0);
        centerPrintC(settings.sudden_death_mode ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_DUD_SETTINGS: {
         centerPrintC("Dud Bomb Config", 0);
         centerPrintC(settings.dud_enabled ? "Status: ENABLED" : "Status: DISABLED", 1);
         centerPrint(String("Chance: ") + settings.dud_chance + "%", 2);
         centerPrintC("#=Tog 1=Set% *=Bk", 3);
      } break;

      case MENU_DUD_CHANCE: {
         centerPrintC("Set Dud Chance %", 0);
         centerPrintC("(1-100)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save)", 3);
      } break;

      case MENU_EXTRAS_SUBMENU: {
         centerPrintC("SETTINGS", 0);
         clearRow(1); lcd.setCursor(0,1); lcd.print("1 Servo  2 EastEggs");
         clearRow(2); lcd.setCursor(0,2); lcd.print("3 Strobe");
         clearRow(3); lcd.setCursor(0,3); lcd.print("* Back");
      } break;

      case MENU_SERVO_SETTINGS: {
        centerPrintC("SERVO CONFIG", 0);
        clearRow(1); lcd.setCursor(0,1); lcd.print("1 Status: "); lcd.print(settings.servo_enabled ? "ON" : "OFF");
        clearRow(2); lcd.setCursor(0,2); lcd.print("2 Start: "); lcd.print(settings.servo_start_angle);
        clearRow(3); lcd.setCursor(0,3); lcd.print("3 End: "); lcd.print(settings.servo_end_angle);
      } break;

      case MENU_SERVO_TOGGLE: {
        centerPrintC("Servo Enabled?", 0);
        centerPrintC(settings.servo_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_SERVO_START_ANGLE: {
         centerPrintC("Set Start Angle", 0);
         centerPrintC("(0-180)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save)", 3);
      } break;

      case MENU_SERVO_END_ANGLE: {
         centerPrintC("Set End Angle", 0);
         centerPrintC("(0-180)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save)", 3);
      } break;

      case MENU_TOGGLE_EASTER_EGGS: {
        centerPrintC("Easter Eggs?", 0);
        centerPrintC(settings.easter_eggs_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_TOGGLE_STROBE: {
        centerPrintC("Explosion Strobe?", 0);
        centerPrintC(settings.explosion_strobe_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_VIEW_RFIDS: {
        centerPrintC("Registered RFID Tags", 0);
        if (rfidViewIndex < settings.num_rfid_uids) {
          const Settings::TagUID &t = settings.rfid_uids[rfidViewIndex];
          centerPrint(String(rfidViewIndex + 1) + ": " + UIDUtil::toHex(t.bytes, t.len), 1);
        } else if (rfidViewIndex == settings.num_rfid_uids) {
          centerPrintC("> Add New Tag <", 1);
        } else {
          centerPrintC("> Clear All Tags <", 1);
        }
        centerPrintC("(#=Select, *=Back)", 3);
      } break;

      case MENU_NETWORK: {
        centerPrintC("NETWORK (Pg 1/2)", 0);
        clearRow(1); lcd.setCursor(0,1); lcd.print("1 WiFi: "); lcd.print(settings.wifi_enabled ? "ON" : "OFF");
        clearRow(2); lcd.setCursor(0,2); lcd.print("2 Mode: "); lcd.print(settings.net_use_mdns ? "mDNS" : "IP");
        clearRow(3); lcd.setCursor(0,3); lcd.print("3 IP 4 Port 9 Next");
      } break;

     case MENU_NETWORK_2: {
        centerPrintC("NETWORK (Pg 2/2)", 0);
        clearRow(1); lcd.setCursor(0,1); lcd.print("5 MasterIP: "); lcd.print(ipToString(settings.master_ip));
        clearRow(2); lcd.setCursor(0,2); lcd.print("6 Setup 7 Apply");
        clearRow(3); lcd.setCursor(0,3); lcd.print("8 Forget  9 Back");
      } break;

      case MENU_NET_ENABLE: {
        centerPrintC("WiFi Enabled?", 0);
        centerPrintC(settings.wifi_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_NET_SERVER_MODE: {
        centerPrintC("Server Mode", 0);
        centerPrintC(settings.net_use_mdns ? "mDNS: scoreboard" : "Static IP", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_NET_IP: {
        centerPrintC("Set Scoreboard IP", 0);
        String ipStr = String(configInputBuffer);
        centerPrint(ipStr.length() ? ipStr : ipToString(settings.scoreboard_ip), 2);
        centerPrintC("(#=Save, *=Back)", 3);
      } break;

      case MENU_NET_PORT: {
        centerPrintC("Set Port", 0);
        centerPrintC(configInputBuffer, 2);
        centerPrintC("(#=Save, *=Back)", 3);
      } break;

      case MENU_NET_MASTER_IP: {
        centerPrintC("Set Master IP", 0);
        String ipStr = String(configInputBuffer);
        centerPrint(ipStr.length() ? ipStr : ipToString(settings.master_ip), 2);
        centerPrintC("(#=Save, *=Back)", 3);
      } break;

      case MENU_NET_WIFI_SETUP: {
        centerPrintC("WiFi Portal: ON", 0);
        centerPrintC("AP: C4Prop-Setup", 1);
        centerPrintC("Join & set creds", 2);
        centerPrintC("(* to stop)", 3);
      } break;

      case MENU_NET_APPLY_NOW: {
        centerPrintC("Applying Net...", 1);
        centerPrintC("Reconfiguring...", 2);
      } break;

      case MENU_SAVE_EXIT: {
        centerPrintC("Configuration", 0);
        centerPrintC("Saving...", 1);
        centerPrintC("Device will reboot.", 2);
      } break;

      case MENU_EXIT_NO_SAVE: {
        centerPrintC("Configuration", 0);
        centerPrintC("DISCARDING...", 1);
        centerPrintC("Device will reboot.", 2);
      } break;

      case MENU_NET_FORGET_CONFIRM: {
        centerPrintC("FORGET WiFi CREDS?", 0);
        centerPrintC("This clears SSID/PWD", 1);
        centerPrintC("(#=Yes, *=No)", 2);
        clearRow(3);
      } break;
      default: break;
    }
    return;
  }

  // ---------- Normal Gameplay Overlay ----------
  if (currentState >= ARMED && currentState < DISARMED) {
    int32_t remaining_ms = (int32_t)settings.bomb_duration_ms - (int32_t)(millis() - bombArmedTimestamp);
    if (remaining_ms < 0) remaining_ms = 0;
    int seconds = remaining_ms / 1000;
    int tenths  = (remaining_ms % 1000) / 100;
    char buffer[21]; snprintf(buffer, sizeof(buffer), "Time: %02d.%d", seconds, tenths);
    centerPrintC(buffer, 0);

    if (currentState == DISARMING_MANUAL || currentState == DISARMING_RFID) {
      static unsigned long lastRandomDigitUpdate = 0;
      static char randomDigit = '0';
      if (millis() - lastRandomDigitUpdate > RANDOM_DIGIT_UPDATE_MS) {
        lastRandomDigitUpdate = millis();
        randomDigit = (char)random('0', '9' + 1);
      }
      long disarm_duration = (currentState == DISARMING_MANUAL) ? settings.manual_disarm_time_ms : settings.rfid_disarm_time_ms;
      long elapsed_disarm  = millis() - disarmStartTimestamp;
      if (elapsed_disarm < 0) elapsed_disarm = 0;
      long time_per_digit  = max<long>(1, disarm_duration / CODE_LENGTH);
      int  digits_revealed = min<int>(CODE_LENGTH, elapsed_disarm / time_per_digit);

      String formattedCode = "";
      for (int i = 0; i < CODE_LENGTH; i++) {
        if (i < digits_revealed)        formattedCode += activeArmCode[i];
        else if (i == digits_revealed)  formattedCode += randomDigit;
        else                             formattedCode += "*";
        if (i < CODE_LENGTH - 1)        formattedCode += " ";
      }
      centerPrint(formattedCode, 2);
      centerPrintC((currentState == DISARMING_MANUAL) ? "Manual Disarm..." : "RFID Disarm...", 1);
      clearRow(3);
    } else {
      switch (currentState) {
        case ARMED:
          centerPrintC("BOMB HAS BEEN", 1);
          centerPrintC("PLANTED", 2);
          clearRow(3);
          break;
        case DISARMING_KEYPAD: {
          centerPrintC("Disarm Code:", 1);
          String formattedCode = "";
          int codeLen = strlen(enteredCode);
          for (int i = 0; i < CODE_LENGTH; i++) {
            if (i < CODE_LENGTH - codeLen) formattedCode += "*";
            else formattedCode += enteredCode[i - (CODE_LENGTH - codeLen)];
            if (i < CODE_LENGTH - 1) formattedCode += " ";
          }
          centerPrint(formattedCode, 2);
          clearRow(3);
        } break;
        default: break;
      }
    }
  } else if (displayNeedsUpdate) {
    displayNeedsUpdate = false;
    lcd.clear(); yield();

    switch (currentState) {
      case STANDBY:
        centerPrintC("SYSTEM OFFLINE", 1);
        break;

      case AWAIT_ARM_TOGGLE:
        centerPrintC("ARM SWITCH IS ON", 1);
        centerPrintC("PLEASE TOGGLE OFF", 2);
        break;

      case PROP_IDLE:
        centerPrintC("System Activated", 0);
        centerPrintC("Enter Arming Code:", 2);
        break;
        
      case STARWARS_PRE_GAME:
        centerPrintC("STAR WARS MODE", 0);
        centerPrintC("Saber FX Ready", 1);
        centerPrintC("# = ARM BOMB", 2);
        centerPrintC("* = CANCEL", 3);
        break;

      case ARMING: {
        centerPrintC("Arming Code:", 1);
        String formattedCode;
        int codeLen = strlen(enteredCode);
        for (int i = 0; i < CODE_LENGTH; i++) {
          if (i < CODE_LENGTH - codeLen) formattedCode += "*";
          else formattedCode += enteredCode[i - (CODE_LENGTH - codeLen)];
          if (i < CODE_LENGTH - 1) formattedCode += " ";
        }
        centerPrint(formattedCode, 2);
        centerPrintC("(# to confirm)", 3);
      } break;

      case DISARMING_KEYPAD:  
        centerPrintC("Disarm Code:", 1);
        {
          String formattedCode;
          int codeLen = strlen(enteredCode);
          for (int i = 0; i < CODE_LENGTH; i++) {
            if (i < CODE_LENGTH - codeLen) formattedCode += "*";
            else formattedCode += enteredCode[i - (CODE_LENGTH - codeLen)];
            if (i < CODE_LENGTH - 1) formattedCode += " ";
          }
          centerPrint(formattedCode, 2);
        }
        clearRow(3);
        break;

      case DISARMED:
        centerPrintC("BOMB HAS BEEN", 1);
        centerPrintC("DEFUSED", 2);
        break;

      case PRE_EXPLOSION:
        centerPrintC("!!! WARNING !!!", 1);
        break;

      case EXPLODED:
        centerPrintC("!!! DETONATED !!!", 1);
        break;

      case EASTER_EGG:
        centerPrintC("<EASTER EGG>", 1);
        centerPrintC("ACTIVATED!", 2);
        break;

      case EASTER_EGG_2:
        centerPrintC("5318008", 1);
        centerPrintC("ARMING...", 2);
        break;
        
      case PROP_DUD:
        centerPrintC("FAILED TO", 1);
        centerPrintC("DETONATE", 2);
        break;

      default: break;
    }
  }
}

// --- LED Logic ---

inline void updateLeds() {
  // --- 1. STATUS LED (Index 0) ---
  switch (currentState) {
    case STANDBY:
    case AWAIT_ARM_TOGGLE: leds[0] = CRGB::Black; break;
    case PROP_IDLE:
    case ARMING: leds[0] = CRGB::Yellow; break;
    case ARMED: 
      if (easterEggActive) {
        // Easter Egg Logic: Cycle colors if the flag is active
        int cycle = (millis() / EASTER_EGG_CYCLE_MS) % 3;
        leds[0] = (cycle==0)?CRGB::Red: (cycle==1)?CRGB::Green: CRGB::Blue;
      } else {
        // Standard Logic: Red Blink
        leds[0] = ledIsOn ? CRGB::Red : CRGB::Black;
      }
      break;
    case DISARMING_KEYPAD:
    case DISARMING_MANUAL:
    case DISARMING_RFID: leds[0] = CRGB::Blue; break;
    case DISARMED: leds[0] = CRGB::Green; break;
    case PRE_EXPLOSION: {
      uint32_t fade = millis() - stateEntryTimestamp;
      uint8_t b = (fade >= PRE_EXPLOSION_FADE_MS) ? 255 : (uint8_t)((fade * 255UL) / PRE_EXPLOSION_FADE_MS);
      leds[0] = CRGB(b,0,0);
    } break;
    case EXPLODED: leds[0] = CRGB::Red; break;
    case EASTER_EGG: {
      // Logic handled in ARMED case via flag, but kept here for initial transition
      int cycle = (millis() / EASTER_EGG_CYCLE_MS) % 3;
      leds[0] = (cycle==0)?CRGB::Red: (cycle==1)?CRGB::Green: CRGB::Blue;
    } break;
    case STARWARS_PRE_GAME: {
      // Slow pulse Green/Red
      int cycle = (millis() / 500) % 2;
      leds[0] = (cycle==0) ? CRGB::Red : CRGB::Green;
    } break;
    case EASTER_EGG_2:
      leds[0] = CRGB::HotPink; 
      break;
    case PROP_DUD:
      // Custom effect: Toggle Purple/Orange
      leds[0] = ((millis() / 250) % 2 == 0) ? CRGB::Purple : CRGB::Orange;
      break;
    case CONFIG_MODE: leds[0] = CRGB::DeepPink; break;
    default: break;
  }

  // --- 2. EXTERIOR STRIP (Indices 1 to NUM_LEDS-1) ---
  if (NUM_LEDS > 1) {
    
    // A. STAR WARS PRE-GAME FX
    if (currentState == STARWARS_PRE_GAME) {
       // Random flashes to simulate saber clashes or "ready" state
       if (random(10) == 0) {
          int pos = random(1, NUM_LEDS);
          leds[pos] = (random(2)==0) ? CRGB::Green : CRGB::Red;
       } else {
          fadeToBlackBy(leds + 1, NUM_LEDS - 1, 40); // Fade trail
       }
    }
    
    // B. EXPLOSION STROBE (Chaotic White Flash)
    else if (currentState == PRE_EXPLOSION) {
       // Only run if enabled
       if (settings.explosion_strobe_enabled) {
          uint32_t elapsed = millis() - stateEntryTimestamp;
          // Delayed Start: 4500ms
          if (elapsed > 4500 && elapsed < 8500) {
             // Much Faster Strobe (Every 50ms)
             bool flash = (millis() / 40) % 2; 
             fill_solid(leds + 1, NUM_LEDS - 1, flash ? CRGB::White : CRGB::Black);
          } else {
             fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
          }
       } else {
          fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
       }
    }
    
    // C. DOOM MODE (Chaotic Hellfire)
    else if (currentState == ARMED && doomModeActive) {
       // 1. Fade everything slightly
       fadeToBlackBy(leds + 1, NUM_LEDS - 1, 100);
       
       // 2. Ignite random spots
       for(int i=0; i<20; i++) { 
          int pos = random(1, NUM_LEDS);
          int colorPick = random(10);
          if (colorPick < 6) leds[pos] = CRGB::Red;
          else if (colorPick < 9) leds[pos] = CRGB::OrangeRed;
          else leds[pos] = CRGB::White; // Spark
       }
    }
    // D. DEFAULT (Off)
    else {
       fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
    }
  }
  FastLED.show();
}