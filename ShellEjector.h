// ShellEjector.h
// VERSION: 1.0.0
// 12.27.2025

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"

// --- CONFIGURATION ---
// Adjust these to match your mechanical setup
static const int      SERVO_NEUTRAL_ANGLE   = 0;    // Resting position
static const int      SERVO_TRIGGER_ANGLE   = 90;   // Angle to hit the shell
static const uint32_t SERVO_TRIGGER_DELAY   = 4000; // Time (ms) to wait AFTER sound starts before hitting
static const uint32_t SERVO_HOLD_TIME       = 300;  // How long (ms) to hold the hit before retracting

// Internal State
Servo myServo;
enum EjectorState { EJECTOR_IDLE, EJECTOR_WAITING, EJECTOR_EXTENDED };
static EjectorState ejectorState = EJECTOR_IDLE;
static uint32_t ejectorTimer = 0;

inline void initShellEjector() {
  // Check if pin is defined to avoid errors if you haven't set it yet
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

// Call this in your main loop()
inline void updateShellEjector() {
  if (ejectorState == EJECTOR_IDLE) return;

  uint32_t elapsed = millis() - ejectorTimer;

  // Phase 1: Waiting for the "Drop" in the audio
  if (ejectorState == EJECTOR_WAITING) {
    if (elapsed >= SERVO_TRIGGER_DELAY) {
      myServo.write(SERVO_TRIGGER_ANGLE);
      ejectorState = EJECTOR_EXTENDED;
      ejectorTimer = millis(); // Reset timer for the hold phase
    }
  }
  // Phase 2: Holding the trigger (pop the shell)
  else if (ejectorState == EJECTOR_EXTENDED) {
    if (elapsed >= SERVO_HOLD_TIME) {
      myServo.write(SERVO_NEUTRAL_ANGLE);
      ejectorState = EJECTOR_IDLE; // Done
    }
  }
}