// ShellEjector.h
// VERSION: 1.2.0
// FIXED: Reliable Trigger Logic

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"

// --- CONFIGURATION ---
static const int      SERVO_NEUTRAL_ANGLE   = 0;    
static const int      SERVO_TRIGGER_ANGLE   = 90;   
// Delay set to 4500 to match Strobe (was 4500)
static const uint32_t SERVO_TRIGGER_DELAY   = 4500; 
static const uint32_t SERVO_HOLD_TIME       = 300; 

// Internal State
Servo myServo;
enum EjectorState { EJECTOR_IDLE, EJECTOR_WAITING, EJECTOR_EXTENDED };
static EjectorState ejectorState = EJECTOR_IDLE;
static uint32_t ejectorTimer = 0;

inline void initShellEjector() {
#ifdef SERVO_PIN
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_NEUTRAL_ANGLE);
  ejectorState = EJECTOR_IDLE;
#endif
}

// Call this immediately when the Explosion Sound starts
inline void startShellEjectorSequence() {
  // Always reset to Idle first to clear any stuck state
  myServo.write(SERVO_NEUTRAL_ANGLE);
  ejectorState = EJECTOR_WAITING;
  ejectorTimer = millis();
  Serial.println("[SERVO] Sequence Started. Waiting 4.5s...");
}

// Call this in your main loop() to handle the animation without blocking
inline void updateShellEjector() {
  if (ejectorState == EJECTOR_IDLE) return;

  uint32_t elapsed = millis() - ejectorTimer;

  // Phase 1: Waiting for the beat drop
  if (ejectorState == EJECTOR_WAITING) {
    if (elapsed >= SERVO_TRIGGER_DELAY) {
      Serial.println("[SERVO] POP!");
      myServo.write(SERVO_TRIGGER_ANGLE);
      ejectorState = EJECTOR_EXTENDED;
      ejectorTimer = millis(); // Reset timer for hold phase
    }
  }
  // Phase 2: Retracting after the hit
  else if (ejectorState == EJECTOR_EXTENDED) {
    if (elapsed >= SERVO_HOLD_TIME) {
      Serial.println("[SERVO] Retract.");
      myServo.write(SERVO_NEUTRAL_ANGLE);
      ejectorState = EJECTOR_IDLE; // Sequence complete
    }
  }
}