// Hardware.h
// VERSION: 3.0.0
// ADDED: Volume control and Sound Toggle support

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
#define NUM_LEDS 60 

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
static const uint32_t AUDIO_COOLDOWN_MS = 100; 

inline void initHardware() {
  // FastLED
  FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN>(leds, NUM_LEDS); 
  FastLED.setBrightness(NEOPIXEL_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black); 
  FastLED.show();

  // LCD
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();

  // RFID
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SDA_PIN);
  rfid.PCD_Init();

  // DFPlayer
  Serial0.begin(9600);
  myDFPlayer.begin(Serial0, true, true);
  myDFPlayer.volume(settings.sound_volume); // Apply saved volume

  // Inputs
  disarmButton.attach(DISARM_BUTTON_PIN, INPUT_PULLUP);
  disarmButton.interval(25);
  armSwitch.attach(ARM_SWITCH_PIN, INPUT_PULLUP);
  armSwitch.interval(25);
  armSwitch.update();

  // Buzzer Setup handled in beepStart/Stop dynamically
}

// --- AUDIO SAFE WRAPPER ---
// Prevents DFPlayer lockups and respects "Sound Enabled" setting
inline void safePlay(int track) {
  if (!settings.sound_enabled) return; // Mute check

  if (millis() - lastAudioCmdTime > AUDIO_COOLDOWN_MS) {
    myDFPlayer.play(track);
    lastAudioCmdTime = millis();
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