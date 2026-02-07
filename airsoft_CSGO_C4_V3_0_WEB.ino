/*
  PROJECT: Counter-Strike C4 Prop (Project 2)
  DATE: 2026-02-03
  AUTHOR: Andrew Florio

  airsoft_CSGO_C4_V3_0_WEB.ino
  VERSION: 4.1.0

  Header-only modular structure for Arduino IDE, with Wi-Fi + mDNS + WebSocket.
  OPTIMIZATION: Throttled LED updates to improve Keypad responsiveness.
  ADDED: Tolkien Mini-Game (Hold '0' on boot)
*/

#include <Arduino.h>

// 1. Basic Definitions (Must come first)
#include "Pins.h"
#include "Config.h"
#include "Sounds.h"

// 2. New Modules
#include "ShellEjector.h"  
#include "PlantSensor.h"   

// 3. Core Logic
#include "Hardware.h" // Defines NUM_LEDS
#include "State.h"         
#include "Utils.h"
#include "Display.h"
#include "Network.h"
#include "Game.h"
#include "TolkienGame.h" // <--- Added

// ---- Default (weak) WS inbound handler ----
__attribute__((weak)) void handleInboundWsMessage(const char* msg) {
  Serial.printf("[WS] RX: %s\n", msg ? msg : "(null)");
}

// ---- Define globals declared in headers ----
hd44780_I2Cexp lcd;
DFRobotDFPlayerMini myDFPlayer;
MFRC522 rfid(RFID_SDA_PIN, RFID_RST_PIN);
Bounce2::Button disarmButton = Bounce2::Button();
Bounce2::Button armSwitch = Bounce2::Button();

CRGB leds[NUM_LEDS]; 

// SERVO DEFINITION
Servo myServo;

EjectorState ejectorState = EJECTOR_IDLE;
uint32_t ejectorTimer = 0;

Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);

// State and config globals
PropState currentState = STANDBY;
ConfigState currentConfigState = MENU_MAIN;

uint32_t bombArmedTimestamp = 0;
uint32_t disarmStartTimestamp = 0;
uint32_t lastBeepTimestamp = 0;
uint32_t stateEntryTimestamp = 0;
uint32_t lastStarPressTime = 0;
bool ledIsOn = false;
bool displayNeedsUpdate = true;
int nextTrackToPlay = 0;
int configMenuIndex = 0;
char configInputBuffer[CONFIG_INPUT_MAX] = "";
int rfidViewIndex = 0;
volatile uint32_t g_restartAtMs = 0;   // for Utils.h deferred restart

bool servoTriggeredThisExplosion = false; 

// Codes
char enteredCode[CODE_LENGTH + 1] = "";
char activeArmCode[CODE_LENGTH + 1] = "";
const char* MASTER_CODE = "7355608";

// Settings storage
Settings settings;

// Network session flags
bool wifiOverrideDisabledThisBoot = false;

void setup() {
  Serial.begin(115200);
  Serial.println("C4 Prop Booting Up...");
  Serial.flush();

  EEPROM.begin(EEPROM_SIZE);
  factoryResetSettingsIfMagicChanged(); // handle struct changes
  loadSettings();

  // Boot overrides: 
  // Hold '*' -> config
  // Hold '#' -> disable WiFi
  // Hold '0' -> Tolkien Game
  keypad.getKey();
  delay(150);
  
  bool starHeld = keypad.isPressed('*');
  bool hashHeld = keypad.isPressed('#');
  bool zeroHeld = keypad.isPressed('0');

  if (starHeld || hashHeld || zeroHeld) {
    uint32_t t0 = millis();
    while (keypad.isPressed('*') || keypad.isPressed('#') || keypad.isPressed('0')) {
      if (millis() - t0 > 1000) break;
    }
  }

  // Handle Boot Flags
  if (hashHeld) {
    wifiOverrideDisabledThisBoot = true;
  }
  
  if (starHeld) {
    currentState = CONFIG_MODE;
    currentConfigState = MENU_MAIN;
    displayNeedsUpdate = true;
  }
  
  // NOTE: Zero held logic is handled at end of setup to allow hardware init first

  initHardware(); 
  initShellEjector();
  initPlantSensor();

  // Boot screen
  lcd.clear();
  lcd.print("C4 Prop Init v");
  lcd.print(FW_VERSION);
  delay(1500);

  // --- CREDITS SCREEN ---
  lcd.clear();
  centerPrintC("Designed by", 0);
  centerPrintC("Andrew Florio", 1);
  centerPrintC("thebassplayer127", 2);
  centerPrintC("@gmail.com", 3);
  delay(2000);

  // Bring up network unless disabled
  beginNetwork(wifiOverrideDisabledThisBoot);

  // Final check for Game Mode
  if (currentState != CONFIG_MODE) {
    if (zeroHeld) {
        startTolkienGame();
    }
    else if (armSwitch.read() == LOW) {
        setState(AWAIT_ARM_TOGGLE);
    } 
    else {
        setState(STANDBY);
    }
  }

  Serial.println("Setup complete.");
}

void loop() {
  char key = keypad.getKey();

  // 1. Service Audio Events
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read());
  }

  // 2. State Logic
  // TOLKIEN GAME overrides normal updates
  if (currentState == TOLKIEN_GAME) {
      serviceTolkienGame(key);
      // We do NOT call updateDisplay() here, as TolkienGame handles its own LCD
  } 
  else if (currentState == CONFIG_MODE) {
      handleConfigMode(key);
      updateDisplay();
  } 
  else {
      disarmButton.update();
      armSwitch.update();
      handleArmSwitch();

      // Timer Logic
      if (currentState >= ARMED && currentState < DISARMED) {
        if (millis() - bombArmedTimestamp >= settings.bomb_duration_ms) setState(PRE_EXPLOSION);
      }

      // Explosion safety guard 
      if (currentState == PRE_EXPLOSION) {
        uint32_t since = millis() - stateEntryTimestamp;
        if (since > (PRE_EXPLOSION_FADE_MS + 10000)) setState(EXPLODED); 
      }

      serviceGameplay(key);
      updateDisplay();
  }
  
  // OPTIMIZATION: Throttle LEDs
  static uint32_t lastLedUpdate = 0;
  if (millis() - lastLedUpdate > 30) {
      updateLeds();
      lastLedUpdate = millis();
  }
  
  menuBeepPump();   
  restartPump();    
  updateShellEjector();
  networkLoop();

  delay(1);
}