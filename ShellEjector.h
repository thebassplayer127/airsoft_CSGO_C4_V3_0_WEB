// ShellEjector.h
// VERSION: 2.0.0
// FIXED: Servo attach/detach logic to prevent jitter and conflicts

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"

// --- FALLBACK CONFIGURATION ---
#ifndef SERVO_PIN
#define SERVO_PIN A2
#endif

// --- CONFIGURATION ---
static const int      SERVO_NEUTRAL_ANGLE   = 0;    
static const int      SERVO_TRIGGER_ANGLE   = 90; // Actuate 90 degrees
static const uint32_t SERVO_HOLD_TIME       = 1000; 

// Internal State
Servo myServo;
enum EjectorState { EJECTOR_IDLE, EJECTOR_EXTENDED, EJECTOR_RETRACTING };
static EjectorState ejectorState = EJECTOR_IDLE;
static uint32_t ejectorTimer = 0;

inline void initShellEjector() {
#ifdef SERVO_PIN
  // DO NOT attach at startup. Attach only when needed.
  // This prevents the servo library from consuming a timer or interfering with LEDC 
  // until the explosion actually happens.
  ejectorState = EJECTOR_IDLE;
  // Ensure pin is output but low
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);
  
  Serial.printf("[SERVO] Initialized (Lazy Attach) on Pin %d\n", SERVO_PIN);
#else
  Serial.println("[SERVO] ERROR: SERVO_PIN NOT DEFINED!");
#endif
}

inline void startShellEjectorSequence() {
  #ifdef SERVO_PIN
    Serial.println("[SERVO] POP! (Attaching & Moving)");
    
    // Attach specifically for this action
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN);
    
    // Move
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
  // Phase 2: Wait for return, then Detach
  else if (ejectorState == EJECTOR_RETRACTING) {
    if (elapsed >= 1000) { 
      Serial.println("[SERVO] Sequence Complete. Detaching.");
      myServo.detach(); // Release resource
      ejectorState = EJECTOR_IDLE;
    }
  }
}