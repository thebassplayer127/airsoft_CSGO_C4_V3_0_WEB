// Hardware.h
//VERSION: 2.1.0
// 12.28.2025

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

  pinMode(BEEP_BUZZER_PIN, OUTPUT);
#if 1 // USE_LEDC_BEEP
  const int BEEP_LEDC_CH = 4;
  ledcAttachPin(BEEP_BUZZER_PIN, BEEP_LEDC_CH);
  ledcWrite(BEEP_LEDC_CH, 0);
#endif
}

// Beeper (LEDC or tone) — using LEDC by default
inline void beepStart(int freqHz) {
  const int BEEP_LEDC_CH = 4;
  ledcWriteTone(BEEP_LEDC_CH, freqHz);
}
inline void beepStop() {
  const int BEEP_LEDC_CH = 4;
  ledcWrite(BEEP_LEDC_CH, 0);
}
