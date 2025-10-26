/*
  PROJECT: Counter-Strike C4 Prop (Project 2)
  VERSION: 3.0.0
  DATE: 2025-10-26
  AUTHOR: Andrew Florio

  Header-only modular structure for Arduino IDE, with Wi-Fi + mDNS + WebSocket.
  DFPlayer remains on Serial0 per wiring (Arduino Nano ESP32).
*/

#include <Arduino.h>
#include "Config.h"
#include "State.h"
#include "Hardware.h"
#include "Utils.h"
#include "Display.h"
#include "Game.h"
#include "Network.h"
#include "Pins.h"
#include "Sounds.h"

// ---- Default (weak) WS inbound handler ----
// Place in Game.h (bottom) OR in your .ino after includes (exactly once).
__attribute__((weak)) void handleInboundWsMessage(const char* msg) {
  // For now just log what came in.
  Serial.printf("[WS] RX: %s\n", msg ? msg : "(null)");
  // TODO: parse JSON or commands here if needed.
}

// ---- Define globals declared in headers ----
hd44780_I2Cexp lcd;
DFRobotDFPlayerMini myDFPlayer;
MFRC522 rfid(RFID_SDA_PIN, RFID_RST_PIN);
Bounce2::Button disarmButton = Bounce2::Button();
Bounce2::Button armSwitch = Bounce2::Button();
CRGB leds[1];
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

  // Boot overrides: hold '*' -> config, hold '#' -> disable WiFi this boot
  keypad.getKey();
  delay(150);
  bool starHeld = keypad.isPressed('*');
  bool hashHeld = keypad.isPressed('#');
  if (starHeld || hashHeld) {
    uint32_t t0 = millis();
    while (keypad.isPressed('*') || keypad.isPressed('#')) {
      if (millis() - t0 > 1000) break;
    }
  }
  if (hashHeld) {
    wifiOverrideDisabledThisBoot = true;
  }
  if (starHeld) {
    currentState = CONFIG_MODE;
    currentConfigState = MENU_MAIN;
    displayNeedsUpdate = true;
  }

  initHardware(); // LCD, FastLED, SPI/I2C, RFID, DFPlayer, buttons, beeper

  // Boot screen with version
  lcd.clear();
  lcd.print("C4 Prop Init v");
  lcd.print(FW_VERSION);
  delay(1000);

  // Bring up network unless disabled
  beginNetwork(wifiOverrideDisabledThisBoot);

  if (currentState != CONFIG_MODE) {
    if (armSwitch.read() == LOW) setState(AWAIT_ARM_TOGGLE);
    else setState(STANDBY);
  }

  Serial.println("Setup complete.");
}

void loop() {
  char key = keypad.getKey();

  if (currentState == CONFIG_MODE) {
    handleConfigMode(key);
  } else {
    disarmButton.update();
    armSwitch.update();
    handleArmSwitch();

    if (myDFPlayer.available()) {
      printDetail(myDFPlayer.readType(), myDFPlayer.read());
    }

    if (currentState >= ARMED && currentState < DISARMED) {
      if (millis() - bombArmedTimestamp >= settings.bomb_duration_ms) setState(PRE_EXPLOSION);
    }

    // Explosion safety guard in case DFPlayer events are missed
    if (currentState == PRE_EXPLOSION) {
      uint32_t since = millis() - stateEntryTimestamp;
      if (since > (PRE_EXPLOSION_FADE_MS + 1200)) {
        if (nextTrackToPlay == SOUND_EXPLOSION_TIMESUP) {
          myDFPlayer.play(SOUND_EXPLOSION_MAIN);
          nextTrackToPlay = 0;
        }
      }
      if (since > (PRE_EXPLOSION_FADE_MS + 4000)) setState(EXPLODED);
    }

    switch (currentState) {
      case PROP_IDLE:
      case ARMING:
        handleKeypadInput(key);
        break;

      case DISARMING_KEYPAD:
        handleKeypadInput(key);
        handleDisarmButton();
        handleRfid();
        handleBeepLogic();  // <<< keep tension beep while keypad-disarming
        break;

      case ARMED:
        handleKeypadInput(key);
        handleDisarmButton();
        handleRfid();
        handleBeepLogic();
        break;

      case DISARMING_MANUAL:
        handleDisarmButton();
        handleBeepLogic();  // <<< keep tension beep while manual-disarming
        if (millis() - disarmStartTimestamp >= settings.manual_disarm_time_ms) setState(DISARMED);
        break;

      case DISARMING_RFID:
        handleBeepLogic();  // <<< keep tension beep while RFID-disarming
        if (millis() - disarmStartTimestamp >= settings.rfid_disarm_time_ms) setState(DISARMED);
        break;

      case EASTER_EGG:
        if (millis() - stateEntryTimestamp > EASTER_EGG_DURATION_MS) {
          bombArmedTimestamp = millis();
          myDFPlayer.play(SOUND_BOMB_PLANTED);
          setState(ARMED);
        }
        break;

      default: break;
    }
  }

  updateDisplay();
  updateLeds();
  menuBeepPump();   // keep menu beeps short & non-blocking
  restartPump();    // perform any scheduled reboot safely

  // Network pump last to keep UI snappy; portal still gets serviced inside.
  networkLoop();

  delay(1);
}
