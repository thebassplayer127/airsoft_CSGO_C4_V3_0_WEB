// Config.h
// VERSION: 4.1.0
// FIXED: Buzzer Frequency set to 2400Hz (Resonant Peak for 12085 Buzzer)

#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// Version
static const char* FW_VERSION = "4.1.0";

// EEPROM / Settings
#define EEPROM_SIZE 512
#define MAX_RFID_UIDS 10
#define SETTINGS_MAGIC    0xC4C40205 
#define SETTINGS_VERSION  5          

struct Settings {
  uint32_t magic_number;
  uint16_t version;
  uint16_t _pad0;

  // Gameplay
  uint32_t bomb_duration_ms;
  uint32_t manual_disarm_time_ms;
  uint32_t rfid_disarm_time_ms;

  // --- MODES ---
  uint8_t  sudden_death_mode;  // 0=Off, 1=On
  uint8_t  dud_enabled;        // 0=Off, 1=On
  uint8_t  dud_chance;         // 1-100%
  
  // --- HARDWARE ---
  uint8_t  servo_enabled;           // 0=Off, 1=On
  uint8_t  servo_start_angle;       // e.g., 0
  uint8_t  servo_end_angle;         // e.g., 90
  
  // --- AUDIO ---
  uint8_t  sound_enabled;           // 0=Off, 1=On
  uint8_t  sound_volume;            // 0-30
  
  // --- SENSORS & EXTRAS ---
  uint8_t  plant_sensor_enabled;    // 0=Off, 1=On
  uint8_t  easter_eggs_enabled;     // 0=Off, 1=On
  uint8_t  explosion_strobe_enabled;// 0=Off, 1=On

  // RFID
  int32_t  num_rfid_uids;
  struct TagUID {
    uint8_t len;
    uint8_t bytes[10];
    uint8_t _pad;
  } rfid_uids[MAX_RFID_UIDS];

  // Network
  uint8_t  wifi_enabled;
  uint8_t  net_use_mdns;
  uint8_t  _pad1[2];
  uint32_t scoreboard_ip;
  uint32_t master_ip;
  uint16_t scoreboard_port;
  uint16_t _pad2;
};

extern Settings settings;

// Gameplay constants
static constexpr int      CODE_LENGTH              = 7;
static constexpr uint32_t DOUBLE_TAP_TIMEOUT      = 500;
static constexpr uint32_t EASTER_EGG_DURATION_MS  = 2000;
static constexpr uint32_t PRE_EXPLOSION_FADE_MS   = 2500;
static constexpr uint32_t RANDOM_DIGIT_UPDATE_MS  = 150;
static constexpr uint32_t EASTER_EGG_CYCLE_MS     = 100;

// Audio / Visual
// FIXED: Updated to 2400Hz to match 12085 Buzzer resonance
static constexpr int      BEEP_TONE_FREQ          = 2400; 
static constexpr uint32_t BEEP_TONE_DURATION_MS   = 225;
static constexpr int      NEOPIXEL_BRIGHTNESS     = 255;
static constexpr size_t   CONFIG_INPUT_MAX        = 16;

// ---------------- Helpers ----------------
inline void factoryResetSettings() {
  memset(&settings, 0, sizeof(settings));
  settings.magic_number            = SETTINGS_MAGIC;
  settings.version                 = SETTINGS_VERSION;

  settings.bomb_duration_ms        = 120000;
  settings.manual_disarm_time_ms   = 15000;
  settings.rfid_disarm_time_ms     = 5000;

  // Gameplay Defaults
  settings.sudden_death_mode       = 0; 
  settings.dud_enabled             = 0; 
  settings.dud_chance              = 5; 
  
  // Audio Defaults 
  settings.sound_enabled           = 1;
  settings.sound_volume            = 20;

  // Hardware Defaults 
  settings.servo_enabled           = 0; 
  settings.servo_start_angle       = 0;
  settings.servo_end_angle         = 90;
  settings.plant_sensor_enabled    = 0;

  // Extras
  settings.easter_eggs_enabled     = 1; 
  settings.explosion_strobe_enabled= 1; 
  
  // Network Defaults 
  settings.num_rfid_uids           = 0;
  settings.wifi_enabled            = 0; 
  settings.net_use_mdns            = 1;
  settings.scoreboard_ip           = (192u<<24) | (168u<<16) | (0u<<8) | 100u;
  settings.master_ip               = (192u<<24) | (168u<<16) | (0u<<8) |  50u;
  settings.scoreboard_port         = 8080;
}

inline bool saveSettings() {
  EEPROM.put(0, settings);
  bool ok = EEPROM.commit();
  if (ok) Serial.println("[CFG] Settings saved to EEPROM.");
  else    Serial.println("[CFG] EEPROM commit FAILED!");
  return ok;
}

inline bool settingsStructValid(const Settings& s) {
  if (s.magic_number != SETTINGS_MAGIC) return false;
  if (s.num_rfid_uids < 0 || s.num_rfid_uids > MAX_RFID_UIDS) return false;
  return true;
}

inline void loadSettings() {
  EEPROM.get(0, settings);
  if (!settingsStructValid(settings)) {
    Serial.println("[CFG] Invalid settings. Factory reset.");
    factoryResetSettings();
    saveSettings();
  } else {
    Serial.println("[CFG] Settings loaded.");
  }
}

inline void factoryResetSettingsIfMagicChanged() {
  Settings tmp; EEPROM.get(0, tmp);
  if (tmp.magic_number != SETTINGS_MAGIC) {
    Serial.println("[CFG] Magic mismatch. Resetting defaults.");
    factoryResetSettings();
    saveSettings();
  }
}