// Display.h
// VERSION: 7.1.0
// ADDED: Delete Card Confirmation Screen

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
  char buf[21];
  memset(buf, ' ', 20); // Fill buffer with spaces
  buf[20] = '\0';       // Null terminator

  int textLength = text.length();
  // Clamp length to 20
  if (textLength > 20) textLength = 20;

  int padding = (20 - textLength) / 2; 
  if (padding < 0) padding = 0;

  // Copy string into the middle of the whitespace buffer
  memcpy(buf + padding, text.c_str(), textLength);

  lcd.setCursor(0, row);
  lcd.print(buf);
}

inline void centerPrintC(const char* text, int row) {
  char buf[21];
  memset(buf, ' ', 20); // Fill buffer with spaces
  buf[20] = '\0';       // Null terminator

  int textLength = strlen(text);
  if (textLength > 20) textLength = 20;

  int padding = (20 - textLength) / 2; 
  if (padding < 0) padding = 0;

  memcpy(buf + padding, text, textLength);

  lcd.setCursor(0, row);
  lcd.print(buf);
}

inline String boolToOnOff(uint8_t v){ return v?String("ON"):String("OFF"); }
inline String modeToStr(uint8_t v){ return v?String("mDNS"):String("StaticIP"); }

// --- Main Display Logic ---

inline void updateDisplay() {
  static uint32_t lastDisplayUpdate = 0;
  uint32_t now = millis();

  // 1. CONFIG MODE (Menu System)
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
          "Fixed Code", 
          "Sudden Death", "Dud Settings", 
          "RFID Tags", "Hardware & Audio", "Network",
          "Save & Exit", "Exit" 
        };
        const int TOTAL = 11; 

        for (int i = -1; i <= 1; ++i) {
          int row = i + 2;
          int itemIndex = (configMenuIndex + i + TOTAL) % TOTAL;
          char lineBuf[21];
          memset(lineBuf, ' ', 20); lineBuf[20] = 0;
          
          String itemText = items[itemIndex];
          if (i == 0) { // Selected item
             lineBuf[0] = '>';
             lineBuf[1] = ' ';
             strncpy(lineBuf + 2, itemText.c_str(), min((int)itemText.length(), 18));
          } else {
             strncpy(lineBuf + 2, itemText.c_str(), min((int)itemText.length(), 18));
          }
          lcd.setCursor(0, row);
          lcd.print(lineBuf);
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

      // FIXED CODE MENUS
      case MENU_FIXED_CODE_SETTINGS: {
        centerPrintC("FIXED CODE MODE", 0);
        lcd.setCursor(0,1); lcd.print("1 Enable: "); lcd.print(settings.fixed_code_enabled ? "ON " : "OFF");
        lcd.setCursor(0,2); lcd.print("2 Set Code"); 
        centerPrint(String("Val: ") + settings.fixed_code_val, 3);
      } break;
      case MENU_FIXED_CODE_TOGGLE: {
        centerPrintC("Fixed Code Req?", 0);
        centerPrintC(settings.fixed_code_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;
      case MENU_SET_FIXED_CODE: {
        centerPrintC("Set Fixed Code", 0);
        centerPrintC("(Max 7 Digits)", 1);
        char buffer[21]; snprintf(buffer, sizeof(buffer), ": %s", configInputBuffer);
        centerPrintC(buffer, 2);
        centerPrintC("(#=Save *=Back)", 3);
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
         centerPrintC("(#=Save *=Back)", 3); 
      } break;

      case MENU_HARDWARE_SUBMENU: {
         centerPrintC("HARDWARE CONFIG", 0);
         lcd.setCursor(0,1); lcd.print("1 Audio  2 Servo    ");
         lcd.setCursor(0,2); lcd.print("3 Sensor 4 FX/Xtras ");
         lcd.setCursor(0,3); lcd.print("* Back              ");
      } break;

      case MENU_AUDIO_SUBMENU: {
         centerPrintC("AUDIO CONFIG", 0);
         lcd.setCursor(0,1); lcd.print("1 Sound: "); lcd.print(settings.sound_enabled ? "ON " : "OFF"); lcd.print("        ");
         lcd.setCursor(0,2); lcd.print("2 Volume: "); lcd.print(settings.sound_volume); lcd.print("        ");
         lcd.setCursor(0,3); lcd.print("* Back              ");
      } break;

      case MENU_AUDIO_TOGGLE: {
        centerPrintC("Sound Enabled?", 0);
        centerPrintC(settings.sound_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      case MENU_VOLUME: {
         centerPrintC("Set Volume", 0);
         centerPrintC("(0-30)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save *=Back)", 3); 
      } break;

      case MENU_PLANT_SENSOR_TOGGLE: {
        centerPrintC("Plant Sensor Enable", 0);
        centerPrintC(settings.plant_sensor_enabled ? "Currently: ON" : "Currently: OFF", 1);
        centerPrintC("(#=Toggle, *=Back)", 2);
      } break;

      // --- EFFECTS / XTRAS ---
      case MENU_EXTRAS_SUBMENU: { 
         centerPrintC("EFFECTS", 0);
         lcd.setCursor(0,1); lcd.print("1 Strobe: "); lcd.print(settings.explosion_strobe_enabled ? "ON " : "OFF");
         lcd.setCursor(0,2); lcd.print("2 Eggs: "); lcd.print(settings.easter_eggs_enabled ? "ON " : "OFF");
         lcd.setCursor(0,3); lcd.print("3 Homing Ping  *Back"); 
      } break;

      // PING MENU
      case MENU_PING_SETTINGS: {
        centerPrintC("HOMING PING", 0);
        lcd.setCursor(0,1); lcd.print("1 Enable: "); lcd.print(settings.ping_enabled ? "ON " : "OFF");
        lcd.setCursor(0,2); lcd.print("2 Time: "); lcd.print(settings.ping_interval_s); lcd.print("s");
        lcd.setCursor(0,3); lcd.print("3 Light: "); lcd.print(settings.ping_light_enabled ? "ON " : "OFF");
      } break;
      case MENU_PING_TOGGLE: {
         centerPrintC("Enable Ping?", 0);
         centerPrintC(settings.ping_enabled ? "Status: ON" : "Status: OFF", 1);
         centerPrintC("(#=Toggle *=Back)", 3);
      } break;
      case MENU_PING_INTERVAL: {
         centerPrintC("Set Ping Interval", 0);
         centerPrintC("(Seconds)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save *=Back)", 3);
      } break;
      case MENU_PING_LIGHT_TOGGLE: {
         centerPrintC("Ping Light Flash?", 0);
         centerPrintC(settings.ping_light_enabled ? "Status: ON" : "Status: OFF", 1);
         centerPrintC("(#=Toggle *=Back)", 3);
      } break;

      case MENU_SERVO_SETTINGS: {
        centerPrintC("SERVO CONFIG", 0);
        lcd.setCursor(0,1); lcd.print("1 Svo:"); lcd.print(settings.servo_enabled ? "ON " : "OFF"); lcd.print("           ");
        lcd.setCursor(0,2); lcd.print("2 Str:"); lcd.print(settings.servo_start_angle);
        lcd.print(" 3 End:"); lcd.print(settings.servo_end_angle); lcd.print("  ");
        lcd.setCursor(0,3); lcd.print("* Back              ");
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
         centerPrintC("(#=Save *=Back)", 3); 
      } break;

      case MENU_SERVO_END_ANGLE: {
         centerPrintC("Set End Angle", 0);
         centerPrintC("(0-180)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save *=Back)", 3); 
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
        centerPrintC("Registered Cards", 0);
        if (rfidViewIndex < settings.num_rfid_uids) {
          const Settings::TagUID &t = settings.rfid_uids[rfidViewIndex];
          String typeStr = (t.type == 1) ? "[ARM]" : "[DIS]";
          centerPrint(typeStr + " " + UIDUtil::toHex(t.bytes, t.len), 1);
          centerPrint(String("Tag ") + (rfidViewIndex + 1) + "/" + settings.num_rfid_uids, 2);
        } else if (rfidViewIndex == settings.num_rfid_uids) {
          centerPrintC("> Add New Tag <", 1);
        } else if (rfidViewIndex == settings.num_rfid_uids + 1) {
          centerPrintC("> Adv. Settings <", 1);
        } else {
          centerPrintC("> Clear All Tags <", 1);
        }
        centerPrintC("(#=Select, *=Back)", 3);
      } break;

      case MENU_DELETE_RFID_CONFIRM: {
         centerPrintC("DELETE THIS CARD?", 0);
         const Settings::TagUID &t = settings.rfid_uids[rfidViewIndex];
         String uid = UIDUtil::toHex(t.bytes, t.len);
         centerPrint(uid, 1);
         centerPrintC("(#=YES, *=NO)", 3);
      } break;

      case MENU_ADD_RFID_SELECT_TYPE: {
        centerPrintC("New Card Function?", 0);
        centerPrintC("1: DISARM KEY", 1);
        centerPrintC("2: ARMING KEY", 2);
        centerPrintC("(*=Cancel)", 3);
      } break;

      case MENU_ADD_RFID_WAIT: {
        centerPrintC("SCAN TAG NOW...", 1);
        centerPrintC("(*=Cancel)", 3);
      } break;

      case MENU_RFID_ADV_SETTINGS: {
        centerPrintC("RFID ARM SETTINGS", 0);
        lcd.setCursor(0,1); lcd.print("1 Mode: "); lcd.print(settings.rfid_arming_mode ? "RANDOM" : "FIXED ");
        lcd.setCursor(0,2); lcd.print("2 Speed: "); lcd.print(settings.rfid_entry_speed_ms); lcd.print("ms");
        lcd.setCursor(0,3); lcd.print("* Back");
      } break;

      case MENU_RFID_ARMING_MODE: {
        centerPrintC("Arming Code Logic", 0);
        centerPrintC(settings.rfid_arming_mode ? "Current: RANDOM" : "Current: FIXED CODE", 1);
        centerPrintC("(#=Toggle *=Back)", 3);
      } break;
      case MENU_RFID_ENTRY_SPEED: {
         centerPrintC("Typing Speed (ms)", 0);
         centerPrintC("(0=Instant)", 1);
         char buffer[21]; snprintf(buffer, sizeof(buffer), "Val: %s", configInputBuffer);
         centerPrintC(buffer, 2);
         centerPrintC("(#=Save *=Back)", 3); 
      } break;

      case MENU_NETWORK: {
        centerPrintC("NETWORK (Pg 1/2)", 0);
        lcd.setCursor(0,1); lcd.print("1 WiFi: "); lcd.print(settings.wifi_enabled ? "ON " : "OFF"); lcd.print("       ");
        lcd.setCursor(0,2); lcd.print("2 Mode: "); lcd.print(settings.net_use_mdns ? "mDNS" : "IP  "); lcd.print("       ");
        lcd.setCursor(0,3); lcd.print("3 IP 4 Port 9 Next  ");
      } break;

     case MENU_NETWORK_2: {
        centerPrintC("NETWORK (Pg 2/2)", 0);
        lcd.setCursor(0,1); lcd.print("5 MastIP: "); lcd.print(ipToString(settings.master_ip));
        lcd.setCursor(0,2); lcd.print("6 Setup 7 Apply     ");
        lcd.setCursor(0,3); lcd.print("8 Forget  9 Back    ");
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
        centerPrintC("Exiting Menu...", 0);
        centerPrintC("(Changes Kept for", 1);
        centerPrintC(" this session)", 2);
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
    if (!displayNeedsUpdate && (now - lastDisplayUpdate < 50)) {
        return; 
    }
    lastDisplayUpdate = now;

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
  } 
  
  // ---------- Static Status Screens (Non-Timing) ----------
  else if (displayNeedsUpdate) {
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
        // Show auto-typing progress if active
        if (autoTypingActive) {
            String formattedCode;
            int codeLen = strlen(enteredCode);
            for (int i = 0; i < CODE_LENGTH; i++) {
               if (i < codeLen) formattedCode += enteredCode[i];
               else formattedCode += "_";
               if (i < CODE_LENGTH-1) formattedCode += " ";
            }
            centerPrint(formattedCode, 3);
        }
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
  uint32_t now = millis();

  // --- 1. STATUS LED (Index 0) ---
  switch (currentState) {
    case STANDBY:
    case AWAIT_ARM_TOGGLE: 
      leds[0] = CRGB::Black; 
      break;
      
    case PROP_IDLE:
      // Yellow Pulse
      leds[0] = CRGB(beatsin8(30, 50, 255), beatsin8(30, 40, 200), 0);
      break;
      
    case ARMING: 
      leds[0] = CRGB::Yellow; 
      break;
      
    case ARMED: 
      if (easterEggActive) {
        int cycle = (millis() / EASTER_EGG_CYCLE_MS) % 3;
        leds[0] = (cycle==0)?CRGB::Red: (cycle==1)?CRGB::Green: CRGB::Blue;
      } else {
        leds[0] = ledIsOn ? CRGB::Red : CRGB::Black;
      }
      break;
      
    case DISARMING_KEYPAD:
    case DISARMING_MANUAL:
    case DISARMING_RFID: 
      leds[0] = CRGB::Blue; 
      break;
      
    case DISARMED: 
      leds[0] = CRGB::Green; 
      break;
      
    case PRE_EXPLOSION: {
      uint32_t fade = millis() - stateEntryTimestamp;
      uint8_t b = (fade >= PRE_EXPLOSION_FADE_MS) ? 255 : (uint8_t)((fade * 255UL) / PRE_EXPLOSION_FADE_MS);
      leds[0] = CRGB(b,0,0);
    } break;
    
    case EXPLODED: 
      leds[0] = CRGB::Red; 
      break;
      
    case EASTER_EGG: {
      int cycle = (millis() / EASTER_EGG_CYCLE_MS) % 3;
      leds[0] = (cycle==0)?CRGB::Red: (cycle==1)?CRGB::Green: CRGB::Blue;
    } break;
    
    case STARWARS_PRE_GAME: {
      int cycle = (millis() / 500) % 2;
      leds[0] = (cycle==0) ? CRGB::Red : CRGB::Green;
    } break;
    
    case EASTER_EGG_2:
      leds[0] = CRGB::HotPink; 
      break;
      
    case PROP_DUD:
      leds[0] = ((millis() / 250) % 2 == 0) ? CRGB::Purple : CRGB::Orange;
      break;
      
    case CONFIG_MODE: 
      leds[0] = CRGB::DeepPink; 
      break;
      
    default: break;
  }

  // --- 2. EXTERIOR STRIP (Indices 1 to NUM_LEDS-1) ---
  if (NUM_LEDS > 1) {
    
    // A. COUNTDOWN (Updated: Flash every 3rd LED)
    if (currentState == ARMED && !easterEggActive && !doomModeActive) {
       if (ledIsOn) {
          for(int i=1; i<NUM_LEDS; i++) {
             // Every 3rd LED flashes Red
             if (i % 3 == 0) leds[i] = CRGB::Red;
             else leds[i] = CRGB::Black;
          }
       } else {
          fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
       }
    }
    
    // FIX: Added DISARMING_KEYPAD to Blue Chase
    else if (currentState == DISARMING_MANUAL || currentState == DISARMING_RFID || currentState == DISARMING_KEYPAD) {
        uint8_t pos = (millis() / 50) % NUM_LEDS;
        for(int i=1; i<NUM_LEDS; i++) {
           if ( abs(i - pos) < 3 || abs(i - (pos+NUM_LEDS)) < 3 ) {
              leds[i] = CRGB::Blue;
           } else {
              leds[i].nscale8(200); // fade tail
           }
        }
    }
    
    // C. DISARMED (Solid Green)
    else if (currentState == DISARMED) {
       fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Green);
    }

    // D. PROP IDLE (Ping & Pulse)
    else if (currentState == PROP_IDLE) {
       // Breathing Yellow
       uint8_t val = beatsin8(20, 0, 100);
       fill_solid(leds + 1, NUM_LEDS - 1, CRGB(val, val/2, 0));
       
       // PING FLASH (Override)
       extern uint32_t lastPingTime;
       if (settings.ping_enabled && settings.ping_light_enabled && (millis() - lastPingTime < 200)) {
           fill_solid(leds + 1, NUM_LEDS - 1, CRGB::White);
           leds[0] = CRGB::White; 
       }
    }

    // E. AUTO TYPING (Green Blips)
    else if (autoTypingActive) {
       uint8_t r = random(1, NUM_LEDS);
       leds[r] = CRGB::Green;
       fadeToBlackBy(leds+1, NUM_LEDS-1, 50);
    }
    
    // F. STAR WARS PRE-GAME FX
    else if (currentState == STARWARS_PRE_GAME) {
       if (random(10) == 0) {
          int pos = random(1, NUM_LEDS);
          leds[pos] = (random(2)==0) ? CRGB::Green : CRGB::Red;
       } else {
          fadeToBlackBy(leds + 1, NUM_LEDS - 1, 40); 
       }
    }
    
    // G. EXPLOSION STROBE / DOOM
    else if (currentState == PRE_EXPLOSION || (currentState == ARMED && doomModeActive)) {
       // ... existing logic ...
       if (currentState == ARMED && doomModeActive) {
           fadeToBlackBy(leds + 1, NUM_LEDS - 1, 100);
           for(int i=0; i<20; i++) { 
              int pos = random(1, NUM_LEDS);
              int c = random(10);
              if (c < 6) leds[pos] = CRGB::Red;
              else if (c < 9) leds[pos] = CRGB::OrangeRed;
              else leds[pos] = CRGB::White; 
           }
       } else if (settings.explosion_strobe_enabled) {
          uint32_t elapsed = millis() - stateEntryTimestamp;
          if (elapsed > 4500 && elapsed < 8500) {
             bool flash = (millis() / 40) % 2; 
             fill_solid(leds + 1, NUM_LEDS - 1, flash ? CRGB::White : CRGB::Black);
          } else {
             fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
          }
       } else {
          fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
       }
    }
    
    // H. DEFAULT (Off)
    else {
       fill_solid(leds + 1, NUM_LEDS - 1, CRGB::Black);
    }
  }
  FastLED.show();
}