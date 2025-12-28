// ShellEjector.h
// VERSION: 1.1.0
// SYNCED: Delay matched to Display.h Strobe (4500ms)

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"

// --- CONFIGURATION ---
// Adjust these angles to match your specific mechanical setup
static const int      SERVO_NEUTRAL_ANGLE   = 0;    // Resting position
static const int      SERVO_TRIGGER_ANGLE   = 90;   // Angle to hit the shell

// The "Drop" timing: How long after the explosion sound starts should it fire?
// Updated to 4500 to match the delayed strobe light start.
static const uint32_t SERVO_TRIGGER_DELAY   = 4500; 
// How long to hold the servo in the "hit" position before retracting
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
  ejectorState = EJECTOR_WAITING;
  ejectorTimer = millis();
}

// Call this in your main loop() to handle the animation without blocking
inline void updateShellEjector() {
  if (ejectorState == EJECTOR_IDLE) return;

  uint32_t elapsed = millis() - ejectorTimer;

  // Phase 1: Waiting for the beat drop
  if (ejectorState == EJECTOR_WAITING) {
    if (elapsed >= SERVO_TRIGGER_DELAY) {
      myServo.write(SERVO_TRIGGER_ANGLE);
      ejectorState = EJECTOR_EXTENDED;
      ejectorTimer = millis(); // Reset timer for hold phase
    }
  }
  // Phase 2: Retracting after the hit
  else if (ejectorState == EJECTOR_EXTENDED) {
    if (elapsed >= SERVO_HOLD_TIME) {
      myServo.write(SERVO_NEUTRAL_ANGLE);
      ejectorState = EJECTOR_IDLE; // Sequence complete
    }
  }
}