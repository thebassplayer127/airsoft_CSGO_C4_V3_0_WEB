// PlantSensor.h
// VERSION: 2.0.0
// ADDED: Setting to enable/disable sensor requirement

#pragma once
#include <Arduino.h>
#include "Pins.h"
#include "Config.h"

// Logic: Sensor LOW usually means Magnet Detected (for standard A3144 sensors)
// Change to 'true' if your sensor is HIGH when magnet is present.
static const bool SENSOR_ACTIVE_LOW = true; 

inline void initPlantSensor() {
#ifdef HALL_SENSOR_PIN
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP); // Use Internal Pullup for open-collector sensors
#endif
}

inline bool isBombPlanted() {
  // If disabled in menu, always return true (simulating it is planted)
  if (!settings.plant_sensor_enabled) return true;

#ifdef HALL_SENSOR_PIN
  bool reading = digitalRead(HALL_SENSOR_PIN);
  return (SENSOR_ACTIVE_LOW ? (reading == LOW) : (reading == HIGH));
#else
  // If no hardware installed yet, always return true so you can still play
  return true; 
#endif
}