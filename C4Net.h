// C4Net.h
// VERSION: 1.0.0
#pragma once
#include <Arduino.h>
#include "Config.h"

// Declared in Network/State already (no need to include Network.h here)
void wsSendJson(const String& json);

// ---- Prop -> Scoreboard events ----

inline void c4SendBombPlanted(uint32_t duration_ms) {
  String j = F("{\"eventType\":\"c4_event\",\"c4_status\":\"bombPlanted\",\"bomb_duration_ms\":");
  j += String(duration_ms);
  j += '}';
  wsSendJson(j);
}

inline void c4SendBombDefused() {
  wsSendJson(F("{\"eventType\":\"c4_event\",\"c4_status\":\"bombDefused\"}"));
}

inline void c4SendBombExploded() {
  wsSendJson(F("{\"eventType\":\"c4_event\",\"c4_status\":\"bombExploded\"}"));
}

inline void c4SendTimePenalty(uint32_t remaining_ms) {
  String j = F("{\"eventType\":\"c4_event\",\"c4_status\":\"timePenalty\",\"remaining_ms\":");
  j += String(remaining_ms);
  j += '}';
  wsSendJson(j);
}

// convenience one-liners for state transitions
inline void c4OnEnterArmed()     { c4SendBombPlanted(settings.bomb_duration_ms); }
inline void c4OnEnterDisarmed()  { c4SendBombDefused(); }
inline void c4OnEnterExploded()  { c4SendBombExploded(); }
inline void c4OnTimeCut(uint32_t new_remaining_ms) { c4SendTimePenalty(new_remaining_ms); }
