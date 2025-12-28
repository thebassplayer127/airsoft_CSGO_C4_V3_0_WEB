// PlantSensor.h
// VERSION: 1.0.0
// 12.27.2025

#pragma once
#include <Arduino.h>
#include "Pins.h"

// Logic: Sensor LOW usually means Magnet Detected (for standard A3144 sensors)
// Change to 'true' if your sensor is HIGH when magnet is present.
static const bool SENSOR_ACTIVE_LOW = true; 

inline void initPlantSensor() {
#ifdef HALL_SENSOR_PIN
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP); // Use Internal Pullup for open-collector sensors
#endif
}

inline bool isBombPlanted() {
#ifdef HALL_SENSOR_PIN
  bool reading = digitalRead(HALL_SENSOR_PIN);
  return (SENSOR_ACTIVE_LOW ? (reading == LOW) : (reading == HIGH));
#else
  // If no hardware installed yet, always return true so you can still play
  return true; 
#endif
}