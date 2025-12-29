// ShellEjector.h
// VERSION: 1.9.0
// FIXED: Added Fallback PIN definition to ensure compilation/running

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"

// --- FALLBACK CONFIGURATION ---
// If SERVO_PIN is missing from Pins.h, we force it to A2 here.
#ifndef SERVO_PIN
#define SERVO_PIN A2
#endif

// --- CONFIGURATION ---
static const int      SERVO_NEUTRAL_ANGLE   = 0;    
static const int      SERVO_TRIGGER_ANGLE   = 120; 
static const uint32_t SERVO_TRIGGER_DELAY   = 0;   
static const uint32_t SERVO_HOLD_TIME       = 1000; 

// Internal State
Servo myServo;
enum EjectorState { EJECTOR_IDLE, EJECTOR_WAITING, EJECTOR_EXTENDED, EJECTOR_RETRACTING };
static EjectorState ejectorState = EJECTOR_IDLE;
static uint32_t ejectorTimer = 0;

inline void initShellEjector() {
#ifdef SERVO_PIN
  // 1. Attach at startup
  myServo.setPeriodHertz(50); 
  int ch = myServo.attach(SERVO_PIN);
  
  // Set initial position
  myServo.write(SERVO_NEUTRAL_ANGLE);
  ejectorState = EJECTOR_IDLE;
  
  Serial.printf("[SERVO] Initialized on Pin %d (Channel %d)\n", SERVO_PIN, ch);
#else
  Serial.println("[SERVO] ERROR: SERVO_PIN NOT DEFINED!");
#endif
}

inline void startShellEjectorSequence() {
  #ifdef SERVO_PIN
    Serial.println("[SERVO] POP! (Moving to Trigger Angle)");
    myServo.write(SERVO_TRIGGER_ANGLE);
    ejectorState = EJECTOR_EXTENDED;
    ejectorTimer = millis();
  #endif
}

inline void updateShellEjector() {
  if (ejectorState == EJECTOR_IDLE) return;

  uint32_t elapsed = millis() - ejectorTimer;

  // Phase 1: Holding the "Pop" position
  if (ejectorState == EJECTOR_EXTENDED) {
    if (elapsed >= SERVO_HOLD_TIME) {
      Serial.println("[SERVO] Retracting...");
      myServo.write(SERVO_NEUTRAL_ANGLE);
      ejectorState = EJECTOR_RETRACTING;
      ejectorTimer = millis(); 
    }
  }
  // Phase 2: Wait for return
  else if (ejectorState == EJECTOR_RETRACTING) {
    if (elapsed >= 1000) { 
      Serial.println("[SERVO] Sequence Complete.");
      ejectorState = EJECTOR_IDLE;
    }
  }
}