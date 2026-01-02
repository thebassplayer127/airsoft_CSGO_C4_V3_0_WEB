// ShellEjector.h
// VERSION: 3.1.0
// FIXED: Removed 'static' from myServo to prevent duplicate instances
// FIXED: Servo attaches/detaches cleanly

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Pins.h"
#include "Config.h"

// --- FALLBACK CONFIGURATION ---
#ifndef SERVO_PIN
#define SERVO_PIN A2
#endif

// --- CONFIGURATION ---
static const uint32_t SERVO_HOLD_TIME       = 1000; 

// Internal State
// IMPORTANT: 'extern' tells the compiler this object exists in the .ino file.
// This prevents creating multiple invisible copies of the servo object.
extern Servo myServo; 

enum EjectorState { EJECTOR_IDLE, EJECTOR_EXTENDED, EJECTOR_RETRACTING };
static EjectorState ejectorState = EJECTOR_IDLE;
static uint32_t ejectorTimer = 0;

inline void initShellEjector() {
#ifdef SERVO_PIN
  // Ensure pin is ready, but don't attach yet (silence)
  ejectorState = EJECTOR_IDLE;
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);
  
  Serial.printf("[SERVO] Initialized (Lazy Attach) on Pin %d\n", SERVO_PIN);
#else
  Serial.println("[SERVO] ERROR: SERVO_PIN NOT DEFINED!");
#endif
}

inline void startShellEjectorSequence() {
  #ifdef SERVO_PIN
    if (!settings.servo_enabled) return;

    Serial.println("[SERVO] POP! (Attaching & Moving)");
    
    // 1. Attach
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN);
    
    // 2. Move to "End" (Pop) Angle
    myServo.write(settings.servo_end_angle);
    
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
      // 3. Move back to "Start" (Neutral) Angle
      myServo.write(settings.servo_start_angle);
      ejectorState = EJECTOR_RETRACTING;
      ejectorTimer = millis(); 
    }
  }
  // Phase 2: Wait for return movement, then Detach
  else if (ejectorState == EJECTOR_RETRACTING) {
    if (elapsed >= 1000) { 
      Serial.println("[SERVO] Sequence Complete. Detaching.");
      // 4. Detach to stop hum/jitter
      myServo.detach(); 
      ejectorState = EJECTOR_IDLE;
    }
  }
}