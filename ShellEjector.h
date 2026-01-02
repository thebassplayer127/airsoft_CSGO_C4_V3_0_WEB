// ShellEjector.h
// VERSION: 3.5.0 (Reverted)
// STATUS: Restored 'detach' logic. Servo goes limp when idle.
// NOTE: This fixes "no movement", but "wiggle on start" is expected behavior for open-loop servos.

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
// 'extern' ensures all files share the EXACT SAME variable instance.
extern Servo myServo; 
enum EjectorState { EJECTOR_IDLE, EJECTOR_EXTENDED, EJECTOR_RETRACTING };

extern EjectorState ejectorState;
extern uint32_t ejectorTimer;

inline void initShellEjector() {
#ifdef SERVO_PIN
  ejectorState = EJECTOR_IDLE;
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  // --- Force Reset to Start Position on Boot ---
  if (settings.servo_enabled) {
      Serial.println("[SERVO] Resetting to Start Angle...");
      myServo.setPeriodHertz(50);
      
      // Write target BEFORE attach to minimize jump
      myServo.write(settings.servo_start_angle); 
      myServo.attach(SERVO_PIN);
      
      delay(500); // Give it time to move home
      
      myServo.detach(); // Relax (Go Limp)
  }
  
  Serial.printf("[SERVO] Initialized on Pin %d\n", SERVO_PIN);
#else
  Serial.println("[SERVO] ERROR: SERVO_PIN NOT DEFINED!");
#endif
}

inline void startShellEjectorSequence() {
  #ifdef SERVO_PIN
    if (!settings.servo_enabled) return;

    Serial.println("[SERVO] POP! (Attaching & Moving)");
    
    // 1. Define Pulse Width FIRST
    myServo.write(settings.servo_end_angle);
    
    // 2. Attach (Sends the pulse)
    if (!myServo.attached()) {
      myServo.setPeriodHertz(50);
      myServo.attach(SERVO_PIN);
    }
    
    ejectorState = EJECTOR_EXTENDED;
    ejectorTimer = millis();
  #endif
}

inline void updateShellEjector() {
  if (ejectorState == EJECTOR_IDLE) return;

  uint32_t elapsed = millis() - ejectorTimer;

  // Phase 1: Holding the "Pop" position (End Angle)
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