// Pins.h
//VERSION: 2.0.0

#pragma once
#include <Arduino.h>
#include <Keypad.h>

// Pins
#define ARM_SWITCH_PIN      A6
#define NEOPIXEL_PIN        2   // FastLED uses GPIO number
#define DISARM_BUTTON_PIN   A0
#define RFID_RST_PIN        D9
#define RFID_SDA_PIN        D10 // CS/SS
#define RFID_MOSI_PIN       D11
#define RFID_MISO_PIN       D12
#define RFID_SCK_PIN        D13
#define BEEP_BUZZER_PIN     A7

// Keypad
static const byte KEYPAD_ROWS = 4;
static const byte KEYPAD_COLS = 3;
static byte ROW_PINS[KEYPAD_ROWS] = {D8, D7, D6, D5};
static byte COL_PINS[KEYPAD_COLS] = {D4, D3, D2};
static char KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
