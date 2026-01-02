// Hardware.h
// VERSION: 2.2.0
// FIXED: Buzzer Whine (Detach Pin)
// ADDED: safePlay() to prevent DFPlayer Lockups

#pragma once
#include <Wire.h>
#include <SPI.h>
#include <FastLED.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Bounce2.h>
#include "DFRobotDFPlayerMini.h"
#include "Pins.h"
#include "Config.h"

// --- LED CONFIGURATION ---
// Index 0 = Status LED (Blinking)
// Index 1..N = Exterior Strip
#define NUM_LEDS 60 // Set this to your max count (e.g., 60, 100)

// Externals defined in .ino
extern hd44780_I2Cexp lcd;
extern DFRobotDFPlayerMini myDFPlayer;
extern MFRC522 rfid;
extern Bounce2::Button disarmButton;
extern Bounce2::Button armSwitch;
extern CRGB leds[NUM_LEDS];
extern Keypad keypad;

// Audio Cooldown Vars
static uint32_t lastAudioCmdTime = 0;
static const uint32_t AUDIO_COOLDOWN_MS = 100; // Minimum time between serial commands

inline void initHardware() {
  // FastLED
  FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN>(leds, NUM_LEDS); // Use NUM_LEDS
  FastLED.setBrightness(NEOPIXEL_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black); // Clear all
  FastLED.show();

  // LCD
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();

  // RFID
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SDA_PIN);
  rfid.PCD_Init();

  // DFPlayer on Serial0 per wiring
  Serial0.begin(9600);
  myDFPlayer.begin(Serial0, true, true);
  myDFPlayer.volume(DFPLAYER_VOLUME);

  // Inputs
  disarmButton.attach(DISARM_BUTTON_PIN, INPUT_PULLUP);
  disarmButton.interval(25);
  armSwitch.attach(ARM_SWITCH_PIN, INPUT_PULLUP);
  armSwitch.interval(25);
  armSwitch.update();

  // Buzzer Setup handled in beepStart/Stop dynamically to prevent whine
}

// --- AUDIO SAFE WRAPPER ---
// Prevents DFPlayer lockups by rate-limiting commands
inline void safePlay(int track) {
  if (millis() - lastAudioCmdTime > AUDIO_COOLDOWN_MS) {
    myDFPlayer.play(track);
    lastAudioCmdTime = millis();
  } else {
    // Optional: Log dropped sound or simple ignore
    // Serial.println("[AUDIO] Skipped track (too fast)");
  }
}

// --- BUZZER CONTROL ---
static const int BEEP_LEDC_CH = 4;

inline void beepStart(int freqHz) {
  // Re-attach pin only when needed
  ledcAttachPin(BEEP_BUZZER_PIN, BEEP_LEDC_CH);
  ledcWriteTone(BEEP_LEDC_CH, freqHz);
}

inline void beepStop() {
  // Write 0 AND Detach to ensure absolute silence (no whine)
  ledcWrite(BEEP_LEDC_CH, 0);
  ledcDetachPin(BEEP_BUZZER_PIN); 
}