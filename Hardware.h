// Hardware.h
// VERSION: 3.5.0
// OPTIMIZED: I2C at 400kHz
// FIXED: Buzzer Duty Cycle set to 128 (50%) for MAX VOLUME

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
static const uint32_t AUDIO_COOLDOWN_MS = 200; 

// --- BUZZER CONFIG ---
static const int BEEP_LEDC_CH = 4;
static const int BEEP_LEDC_RES = 8; // 8-bit resolution

inline void initHardware() {
  // FastLED
  FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN>(leds, NUM_LEDS); 
  FastLED.setBrightness(NEOPIXEL_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black); 
  FastLED.show();

  // LCD
  Wire.begin();
  Wire.setClock(400000); // 400kHz Fast Mode
  lcd.begin(20, 4);
  lcd.backlight();

  // RFID
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SDA_PIN);
  rfid.PCD_Init();

  // DFPlayer
  Serial0.begin(9600);
  myDFPlayer.begin(Serial0, true, true);
  myDFPlayer.volume(settings.sound_volume);

  // Inputs
  disarmButton.attach(DISARM_BUTTON_PIN, INPUT_PULLUP);
  disarmButton.interval(25);
  armSwitch.attach(ARM_SWITCH_PIN, INPUT_PULLUP);
  armSwitch.interval(25);
  armSwitch.update();

  // Buzzer Setup (One-time Init)
  ledcAttachPin(BEEP_BUZZER_PIN, BEEP_LEDC_CH);
  ledcSetup(BEEP_LEDC_CH, BEEP_TONE_FREQ, BEEP_LEDC_RES);
  ledcWrite(BEEP_LEDC_CH, 0); // Ensure silence at boot
}

// --- AUDIO SAFE WRAPPER ---
inline void safePlay(int track) {
  if (!settings.sound_enabled) return; 

  if (millis() - lastAudioCmdTime > AUDIO_COOLDOWN_MS) {
    myDFPlayer.play(track);
    lastAudioCmdTime = millis();
  }
}

// --- BUZZER CONTROL ---
inline void beepStart(int freqHz) {
  static int currentFreq = -1;

  if (currentFreq != freqHz) {
    ledcSetup(BEEP_LEDC_CH, freqHz, BEEP_LEDC_RES);
    currentFreq = freqHz;
  }
  
  // MAX VOLUME: 50% Duty Cycle (128/255)
  // This provides the maximum power transfer for a square wave.
  ledcWrite(BEEP_LEDC_CH, 128); 
}

inline void beepStop() {
  ledcWrite(BEEP_LEDC_CH, 0);
}