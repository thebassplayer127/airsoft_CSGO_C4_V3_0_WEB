// Utils.h
//VERSION: 2.1.0
// FIXED: Updated menu sounds to use safePlay()

#pragma once
#include <Arduino.h>
#include <string.h>
#include "Sounds.h"
#include "Hardware.h"

// ---------- Non-blocking menu audio ----------
#ifndef MENU_SOUNDS
#define MENU_SOUNDS 0   // 0=LEDC beeps (safe), 1=DFPlayer tracks
#endif

inline void menuClick(uint16_t trackNav = SOUND_MENU_NAV) {
#if MENU_SOUNDS
  safePlay(trackNav);
#else
  beepStart(2400);
#endif
}
inline void menuConfirm(uint16_t trackOk = SOUND_MENU_CONFIRM) {
#if MENU_SOUNDS
  safePlay(trackOk);
#else
  beepStart(1800);
#endif
}
inline void menuCancel(uint16_t trackErr = SOUND_MENU_CANCEL) {
#if MENU_SOUNDS
  safePlay(trackErr);
#else
  beepStart(800);
#endif
}
inline void menuBeepPump() {
#if !MENU_SOUNDS
  extern bool ledIsOn;
  static unsigned long startedAt = 0;
  static bool running = false;
  if (!running) { if (!ledIsOn) { running = true; startedAt = millis(); } }
  else { if (millis() - startedAt >= 60) {
  if (!ledIsOn) {
    beepStop();
  }
  running = false;
} }
#endif
}

// ---------- IP/UID helpers ----------
inline String ipToString(uint32_t ip) {
  uint8_t a = (ip >> 24) & 0xFF;
  uint8_t b = (ip >> 16) & 0xFF;
  uint8_t c = (ip >> 8)  & 0xFF;
  uint8_t d = ip & 0xFF;
  return String(a) + "." + String(b) + "." + String(c) + "." + String(d);
}
struct UIDUtil {
  static inline bool equals_len_bytes(uint8_t alen, const uint8_t* abytes,
                                      const uint8_t* b, uint8_t blen) {
    if (alen != blen) return false;
    return memcmp(abytes, b, blen) == 0;
  }
  static inline String toHex(const uint8_t* bytes, uint8_t len) {
    char buf[3*10 + 1]; int o = 0;
    for (uint8_t i = 0; i < len; i++) {
      o += snprintf(buf + o, sizeof(buf)-o, "%02X%s", bytes[i], (i + 1 < len) ? ":" : "");
    }
    return String(buf);
  }
};

// ---------- Deferred restart (prevents “freeze on save”) ----------
// ONE shared timer across the sketch (defined in .ino)
extern volatile uint32_t g_restartAtMs;

inline void requestRestart(uint32_t delayMs = 500) {
  uint32_t when = millis() + delayMs;
  g_restartAtMs = when;
  Serial.printf("[REBOOT] requestRestart(%u) scheduled for t=%u ms\n", delayMs, when);
}

inline void restartPump() {
  static uint32_t lastLog = 0;
  if (g_restartAtMs == 0) return;

  int32_t remaining = (int32_t)(g_restartAtMs - millis());
  if ((millis() - lastLog) > 250) {           
    lastLog = millis();
    Serial.printf("[REBOOT] pending… now=%u target=%u remaining=%ld ms\n",
                  (unsigned)millis(), (unsigned)g_restartAtMs, (long)remaining);
  }

  if (remaining <= 0) {
    Serial.println("[REBOOT] firing now: stopping audio/LED, restarting ESP");
    beepStop(); // Ensure silence
    
    // Stop Servo
    extern Servo myServo;
    if (myServo.attached()) myServo.detach();

    delay(50);
    ESP.restart();
  }
}