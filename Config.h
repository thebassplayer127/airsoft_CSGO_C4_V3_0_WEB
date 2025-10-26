// Config.h
//VERSION: 2.3.0

#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// Version (shown on boot and in menu header)
static const char* FW_VERSION = "2.3.0";

// EEPROM / Settings
#define EEPROM_SIZE 512
#define MAX_RFID_UIDS 10
#define SETTINGS_MAGIC    0xC4C40103   // bumped for new struct layout
#define SETTINGS_VERSION  2

struct Settings {
  uint32_t magic_number;        // SETTINGS_MAGIC
  uint16_t version;             // SETTINGS_VERSION
  uint16_t _pad0;               // alignment

  // Gameplay
  uint32_t bomb_duration_ms;
  uint32_t manual_disarm_time_ms;
  uint32_t rfid_disarm_time_ms;

  // RFID
  int32_t  num_rfid_uids;
  struct TagUID {
    uint8_t len;
    uint8_t bytes[10];
    uint8_t _pad;               // explicit padding to 12 bytes
  } rfid_uids[MAX_RFID_UIDS];

  // ---- Network settings ----
  uint8_t  wifi_enabled;        // 0/1
  uint8_t  net_use_mdns;        // 0=static IP, 1=mDNS
  uint8_t  _pad1[2];            // alignment
  uint32_t scoreboard_ip;       // IPv4 packed A.B.C.D (host order)
  uint32_t master_ip;           // IPv4 packed A.B.C.D (host order)
  uint16_t scoreboard_port;     // default 8080
  uint16_t _pad2;               // alignment
};

// Declared in .ino
extern Settings settings;

// Gameplay constants
static constexpr int      CODE_LENGTH              = 7;
static constexpr uint32_t DOUBLE_TAP_TIMEOUT      = 500;  // ms
static constexpr uint32_t EASTER_EGG_DURATION_MS  = 2000;
static constexpr uint32_t PRE_EXPLOSION_FADE_MS   = 2500;
static constexpr uint32_t RANDOM_DIGIT_UPDATE_MS  = 150;
static constexpr uint32_t EASTER_EGG_CYCLE_MS     = 100;

// Audio / Visual
static constexpr int      BEEP_TONE_FREQ          = 2500;
static constexpr uint32_t BEEP_TONE_DURATION_MS   = 125;
static constexpr int      DFPLAYER_VOLUME         = 10;    // 0..30
static constexpr int      NEOPIXEL_BRIGHTNESS     = 200;   // 0..255

// Config input buffer size
static constexpr size_t   CONFIG_INPUT_MAX        = 16;

// ---------------- Helpers ----------------
inline void factoryResetSettings() {
  memset(&settings, 0, sizeof(settings));
  settings.magic_number            = SETTINGS_MAGIC;
  settings.version                 = SETTINGS_VERSION;

  // Gameplay defaults
  settings.bomb_duration_ms        = 45000;
  settings.manual_disarm_time_ms   = 10000;
  settings.rfid_disarm_time_ms     = 5000;
  settings.num_rfid_uids           = 0;

  // Network defaults
  settings.wifi_enabled            = 1;
  settings.net_use_mdns            = 1; // prefer mDNS by default
  settings.scoreboard_ip           = (192u<<24) | (168u<<16) | (0u<<8) | 100u; // 192.168.0.100
  settings.master_ip               = (192u<<24) | (168u<<16) | (0u<<8) |  50u; // 192.168.0.50
  settings.scoreboard_port         = 81;
}

// returns true on success
inline bool saveSettings() {
  EEPROM.put(0, settings);
  bool ok = EEPROM.commit();
  if (ok) Serial.println("[CFG] Settings saved to EEPROM.");
  else    Serial.println("[CFG] EEPROM commit FAILED!");
  return ok;
}

inline bool settingsStructValid(const Settings& s) {
  if (s.magic_number != SETTINGS_MAGIC) return false;
  if (s.version      != SETTINGS_VERSION) return false;
  if (s.num_rfid_uids < 0 || s.num_rfid_uids > MAX_RFID_UIDS) return false;
  return true;
}

inline void loadSettings() {
  EEPROM.get(0, settings);
  if (!settingsStructValid(settings)) {
    Serial.println("[CFG] Invalid/old settings detected. Factory reset.");
    factoryResetSettings();
    saveSettings();
  } else {
    Serial.println("[CFG] Settings loaded from EEPROM.");
  }
}

// Optional one-shot migration helper (safe to call in setup after EEPROM.begin)
inline void factoryResetSettingsIfMagicChanged() {
  Settings tmp; EEPROM.get(0, tmp);
  if (!settingsStructValid(tmp)) {
    Serial.println("[CFG] Magic/version mismatch. Resetting to defaults.");
    factoryResetSettings();
    saveSettings();
  }
}
