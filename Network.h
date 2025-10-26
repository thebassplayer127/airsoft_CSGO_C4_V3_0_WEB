// Network.h
// VERSION: 2.2.0   // consolidated: numeric-IP dialing, WS watchdog, mDNS-once, gameplay gating, cooldown

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebSocketsClient.h>
#include <WiFiManager.h>
#include "Config.h"
#include "State.h"
#include "Utils.h"

// ---- WebSocket dials & headers ----
#ifndef WS_PATH
  #define WS_PATH "/"            // change to "/ws" if your server uses that route
#endif
#ifndef WS_SUBPROTO
  #define WS_SUBPROTO ""         // e.g., "json" if server requires a subprotocol
#endif
#ifndef WS_USE_SSL
  #define WS_USE_SSL 0           // 1 for wss:// (requires proper server TLS setup)
#endif
#ifndef WS_CONNECT_BY_IP
  #define WS_CONNECT_BY_IP 1     // 0 = dial hostname; 1 = dial resolved IP (safer/faster fail)
#endif

// -----------------------------------------------------------------------------
// External functions implemented elsewhere
// -----------------------------------------------------------------------------
void handleInboundWsMessage(const char* msg);  // define in Game.* or your .ino

// -----------------------------------------------------------------------------
// Small helpers (local overload so IPAddress logs correctly)
// -----------------------------------------------------------------------------
inline String ipToString(const IPAddress& ip) { return ip.toString(); }

// -----------------------------------------------------------------------------
// ===== Built-in RGB Status LED (Arduino Nano ESP32 ABX00092) =====
// -----------------------------------------------------------------------------
#ifndef LED_RED
  #define LED_RED   14
#endif
#ifndef LED_GREEN
  #define LED_GREEN 16
#endif
#ifndef LED_BLUE
  #define LED_BLUE  15
#endif

#ifndef LED_SWAP_GB
  #define LED_SWAP_GB 1
#endif
#if LED_SWAP_GB
  #define LEDG_PIN LED_BLUE
  #define LEDB_PIN LED_GREEN
#else
  #define LEDG_PIN LED_GREEN
  #define LEDB_PIN LED_BLUE
#endif

#ifndef LED_INVERT
  #define LED_INVERT 1
#endif

static bool ledInitDone = false;
static const int LEDC_CH_R = 0;
static const int LEDC_CH_G = 1;
static const int LEDC_CH_B = 2;
static const int LEDC_FREQ = 2000;     // Hz
static const int LEDC_RES  = 8;        // bits (0..255)

enum LedMode : uint8_t {
  LEDMODE_OFF = 0,
  LEDMODE_SOLID_RED,
  LEDMODE_SOLID_GREEN,
  LEDMODE_SOLID_BLUE,
  LEDMODE_PULSE_YELLOW
};

static LedMode ledMode = LEDMODE_OFF;

inline void ledInit() {
  if (ledInitDone) return;
  pinMode(LED_RED, OUTPUT);
  pinMode(LEDG_PIN, OUTPUT);
  pinMode(LEDB_PIN, OUTPUT);
  ledcSetup(LEDC_CH_R, LEDC_FREQ, LEDC_RES);
  ledcSetup(LEDC_CH_G, LEDC_FREQ, LEDC_RES);
  ledcSetup(LEDC_CH_B, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(LED_RED, LEDC_CH_R);
  ledcAttachPin(LEDG_PIN, LEDC_CH_G);
  ledcAttachPin(LEDB_PIN, LEDC_CH_B);
  ledInitDone = true;
}

inline void ledWriteRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (!ledInitDone) ledInit();
  if (LED_INVERT) { r = 255 - r; g = 255 - g; b = 255 - b; }
  ledcWrite(LEDC_CH_R, r);
  ledcWrite(LEDC_CH_G, g);
  ledcWrite(LEDC_CH_B, b);
}

inline void ledSetMode(LedMode m) { ledMode = m; }

inline void ledTick() {
  switch (ledMode) {
    case LEDMODE_SOLID_RED:    ledWriteRGB(255,   0,   0); break;
    case LEDMODE_SOLID_GREEN:  ledWriteRGB(  0, 255,   0); break;
    case LEDMODE_SOLID_BLUE:   ledWriteRGB(  0,   0, 255); break;
    case LEDMODE_PULSE_YELLOW: {
      static uint32_t t0 = millis();
      float t = (millis() - t0) / 1000.0f;
      float s = 0.5f * (1.0f + sinf(2.0f * PI * 0.5f * t)); // 0..1 @ 0.5 Hz
      uint8_t v = (uint8_t)(40 + s * 215);                  // 40..255
      ledWriteRGB(v, v, 0);
      break;
    }
    case LEDMODE_OFF:
    default:                   ledWriteRGB(  0,   0,   0); break;
  }
}

// -----------------------------------------------------------------------------
// Session / module state
// -----------------------------------------------------------------------------
static WebSocketsClient wsClient;
static bool wsConnected = false;
static bool wsConnecting = false;           // prevents repeated begin()
static unsigned long wsConnectingSinceMs = 0;   // watchdog for stuck connects
static unsigned long nextWsAttemptMs = 0;       // backoff scheduler

static bool wifiSessionDisabled = false;

static bool mdnsStarted = false;
static unsigned long lastResolveAttemptMs = 0;
static IPAddress cachedScoreboardIP;

static const uint16_t WS_RECONNECT_MS = 7000;
static const uint16_t WS_INITIAL_DELAY_MS = 500;   // slight delay before 1st try

// track when we tried Wi-Fi connect (to decide when to show "red")
static unsigned long lastWifiAttemptMs = 0;
static const unsigned long WIFI_FAIL_GRACE_MS = 15000; // 15s until we deem it "failed"

// WiFiManager portal state
static bool g_portalConnectedThisSession = false;
static WiFiManager g_wm;  // single instance shared across start/loop/stop

// Cooldown after repeated WS failures
static uint8_t  wsConsecutiveFails = 0;
static unsigned long wsFirstFailWindowMs = 0;
static unsigned long wsBackoffUntilMs = 0; // when nonzero, skip attempts

// -----------------------------------------------------------------------------
// Forwards for local helpers (to avoid order issues)
// -----------------------------------------------------------------------------
inline void connectWebSocket();
inline void wsSendJson(const String& json);
inline void startWiFiPortal(uint16_t seconds = 90);
inline void stopWiFiPortal();
inline void networkPortalLoop();
inline void networkReconfigure();
inline bool resolveScoreboardIP();
inline void forgetWifiCredentials();
inline void beginNetwork(bool disableForThisBoot);
inline void networkLoop();

// -----------------------------------------------------------------------------
// WebSocket helpers (connect via resolved IP if we have one)
// -----------------------------------------------------------------------------
inline void connectWebSocket() {
  if (!WiFi.isConnected()) return;
  if (wsConnected || wsConnecting) return;

  // Prefer hostname for log/Host:, but dial numeric IP when allowed
  String hostForLog = settings.net_use_mdns ? "scoreboard.local" : ipToString(settings.scoreboard_ip);
  String tcpHost    = hostForLog;
  if (settings.net_use_mdns && WS_CONNECT_BY_IP && cachedScoreboardIP != IPAddress(0,0,0,0)) {
    tcpHost = ipToString(cachedScoreboardIP); // TCP target is numeric IP
  }

  wsConnecting = true;
  wsConnectingSinceMs = millis();

  Serial.printf("[NET] Connecting WebSocket to %s (%s):%u path=%s\n",
                hostForLog.c_str(), tcpHost.c_str(), settings.scoreboard_port, WS_PATH);

#if WS_USE_SSL
  wsClient.beginSSL(tcpHost.c_str(), settings.scoreboard_port, WS_PATH);
#else
  wsClient.begin(tcpHost.c_str(), settings.scoreboard_port, WS_PATH);
#endif

  if (strlen(WS_SUBPROTO)) {
    String hdr = String("Sec-WebSocket-Protocol: ") + WS_SUBPROTO + "\r\n";
    wsClient.setExtraHeaders(hdr.c_str());
  }

  // Heartbeat: ping every 15s, timeout if no pong in 3s, allow 2 misses
  wsClient.enableHeartbeat(15000, 3000, 2);

  wsClient.onEvent([](WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        wsConnected = true;
        wsConnecting = false;
        wsConnectingSinceMs = 0;
        wsConsecutiveFails = 0;
        wsFirstFailWindowMs = 0;
        wsBackoffUntilMs = 0;
        Serial.println("[NET] WebSocket connected.");
        break;

      case WStype_DISCONNECTED: {
        wsConnected = false;
        wsConnecting = false;
        wsConnectingSinceMs = 0;
        Serial.println("[NET] WebSocket disconnected.");

        unsigned long now = millis();
        if (wsFirstFailWindowMs == 0 || now - wsFirstFailWindowMs > 60000UL) {
          wsFirstFailWindowMs = now;
          wsConsecutiveFails = 1;
        } else {
          wsConsecutiveFails++;
        }
        if (wsConsecutiveFails >= 3) {
          wsBackoffUntilMs = now + 5UL * 60UL * 1000UL; // 5 minutes cooldown
          Serial.println("[NET] Too many WS failures; cooling down for 5 minutes.");
        }

        nextWsAttemptMs = now + WS_RECONNECT_MS; // schedule retry
      } break;

      case WStype_TEXT:
        handleInboundWsMessage((const char*)payload);
        break;

      case WStype_BIN:
        Serial.printf("[NET] WS RX BIN (%u bytes)\n", (unsigned)length);
        break;

      default:
        Serial.printf("[NET] WS event type=%d len=%u\n", (int)type, (unsigned)length);
        break;
    }
  });

  wsClient.setReconnectInterval(WS_RECONNECT_MS);
}

inline void wsSend(const String& s) {
  if (wsConnected) wsClient.sendTXT((const uint8_t*)s.c_str(), s.length());
}
inline void wsSendJson(const String& json) { wsSend(json); }

// -----------------------------------------------------------------------------
// WiFi portal (non-blocking WiFiManager)
// -----------------------------------------------------------------------------
inline void startWiFiPortal(uint16_t seconds) {
  g_wm.setConfigPortalBlocking(false);
  g_wm.setTimeout(seconds);
  g_wm.setBreakAfterConfig(true);  // exit CP after creds saved

  g_wm.setSaveConfigCallback([]() {
    g_portalConnectedThisSession = true;
    Serial.println("[NET] WiFiManager save callback fired.");
  });

  const char* ap = "C4Prop-Setup";
  (void)g_wm.startConfigPortal(ap);  // non-blocking: return means "connected"
  Serial.println("[NET] WiFi portal launched (non-blocking).");

  if (g_wm.getConfigPortalActive()) {
    Serial.printf("[NET] Portal active at http://%s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[NET] Portal not yet active (will tick in loop).");
  }
}

inline void stopWiFiPortal() {
  g_wm.stopConfigPortal();
  Serial.println("[NET] WiFi portal stopped.");
}

inline void networkPortalLoop() {
  if (g_wm.getConfigPortalActive()) g_wm.process();
}

// -----------------------------------------------------------------------------
// Network (re)configuration entry point (invoked from menu "Apply Now")
// -----------------------------------------------------------------------------
inline void networkReconfigure() {
  Serial.println("[NET] Applying network settings…");

  // Ensure portal isn't running to avoid SoftAP/STA conflicts
  stopWiFiPortal();

  // Tear down WebSocket so we can reconnect cleanly with new settings
  wsClient.disconnect();
  wsConnected = false;
  wsConnecting = false;
  wsConnectingSinceMs = 0;
  nextWsAttemptMs = millis() + WS_INITIAL_DELAY_MS;

  // Clear cooldown counters so user actions immediately re-try
  wsBackoffUntilMs = 0;
  wsConsecutiveFails = 0;
  wsFirstFailWindowMs = 0;

  if (!settings.wifi_enabled) {
    wifiSessionDisabled = true;
    mdnsStarted = false;
    WiFi.disconnect(true, true);   // also clears creds from NVS
    WiFi.mode(WIFI_OFF);
    Serial.println("[NET] Networking disabled for this boot.");
    ledSetMode(LEDMODE_PULSE_YELLOW);
    return;
  }

  wifiSessionDisabled = false;
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // Always attempt autoconnect to whatever creds are in NVS
  WiFi.begin();
  lastWifiAttemptMs = millis();

  // (Re)start mDNS once here if not started yet
  if (!mdnsStarted) {
    if (MDNS.begin("c4prop")) {
      mdnsStarted = true;
      Serial.println("[NET] mDNS responder started (hostname c4prop).");
    } else {
      Serial.println("[NET] mDNS start failed; will retry later (not in loop).");
    }
  }

  // First WS attempt will be scheduled a moment later
  nextWsAttemptMs = millis() + WS_INITIAL_DELAY_MS;

  Serial.println("[NET] Reconfigure complete.");
}

// -----------------------------------------------------------------------------
// WiFi credentials management
// -----------------------------------------------------------------------------
inline void forgetWifiCredentials() {
  Serial.println("[NET] Forgetting saved WiFi credentials...");

  if (g_wm.getConfigPortalActive() || (WiFi.getMode() & WIFI_MODE_AP)) {
    Serial.println("[NET] Stopping WiFi portal before erasing creds…");
    stopWiFiPortal();
  }

  WiFi.disconnect(true, true);   // erase config, block STA
  {
    WiFiManager wm;
    wm.resetSettings();
  }

  delay(100);
  Serial.println("[NET] Credentials erased. Use Network->WiFi Setup to configure.");
}

// -----------------------------------------------------------------------------
// Bring up networking at boot (or after disabling)
// -----------------------------------------------------------------------------
inline void beginNetwork(bool disableForThisBoot) {
  ledInit(); // make sure LED is ready

  wifiSessionDisabled = disableForThisBoot || !settings.wifi_enabled;

  if (wifiSessionDisabled) {
    Serial.println("[NET] Networking DISABLED for this boot.");
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    ledSetMode(LEDMODE_PULSE_YELLOW);
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // IMPORTANT: Always try autoconnect using saved credentials.
  Serial.println("[NET] Boot: attempting STA autoconnect (saved creds).");
  WiFi.begin();
  lastWifiAttemptMs = millis();

  // Start mDNS ONCE here (not in loop)
  if (!mdnsStarted) {
    if (MDNS.begin("c4prop")) {
      mdnsStarted = true;
      Serial.println("[NET] mDNS responder started (hostname c4prop).");
    } else {
      Serial.println("[NET] mDNS start failed; will retry later (not in loop).");
    }
  }

  // First WS attempt will be scheduled a moment later
  wsConnected = false;
  wsConnecting = false;
  wsConnectingSinceMs = 0;
  nextWsAttemptMs = millis() + WS_INITIAL_DELAY_MS;

  // keep portal responsive if opened later
  networkPortalLoop();
}

// -----------------------------------------------------------------------------
// Resolve scoreboard host (mDNS or static IP)
// -----------------------------------------------------------------------------
inline bool resolveScoreboardIP() {
  if (!settings.net_use_mdns) {
    cachedScoreboardIP = settings.scoreboard_ip;
    return true;
  }

  const unsigned long RESOLVE_BACKOFF_MS = 5000;
  unsigned long now = millis();
  if (now - lastResolveAttemptMs < RESOLVE_BACKOFF_MS) {
    return (cachedScoreboardIP != IPAddress(0,0,0,0));
  }
  lastResolveAttemptMs = now;

  // Query only; mDNS BEGIN happens in beginNetwork()/networkReconfigure()
  const uint32_t MDNS_QUERY_TIMEOUT_MS = 250;
  IPAddress ip = MDNS.queryHost("scoreboard");
  if (ip == IPAddress(0,0,0,0)) ip = MDNS.queryHost("scoreboard.local");
  if (ip != IPAddress(0,0,0,0)) {
    cachedScoreboardIP = ip;
    Serial.printf("[NET] mDNS resolved scoreboard.local -> %s\n", ipToString(ip).c_str());
    return true;
  }

  return (cachedScoreboardIP != IPAddress(0,0,0,0));
}

// -----------------------------------------------------------------------------
// Main network loop (call from your main loop())
// -----------------------------------------------------------------------------
inline void networkLoop() {
  // LED: decide the *desired* mode from connection state first
  if (wifiSessionDisabled) {
    ledSetMode(LEDMODE_PULSE_YELLOW);
  } else if (wsConnected) {
    ledSetMode(LEDMODE_SOLID_BLUE);   // Wi-Fi + WS OK
  } else if (WiFi.isConnected()) {
    ledSetMode(LEDMODE_SOLID_GREEN);  // Wi-Fi OK, WS not connected yet
  } else {
    if (lastWifiAttemptMs && (millis() - lastWifiAttemptMs > WIFI_FAIL_GRACE_MS)) {
      ledSetMode(LEDMODE_SOLID_RED);
    } else {
      ledSetMode(LEDMODE_OFF);
    }
  }
  ledTick();

  if (wifiSessionDisabled) return;

  // Keep portal responsive if it's active
  networkPortalLoop();

  // If we just saved creds via WiFiManager, attempt a STA connect once
  if (g_portalConnectedThisSession) {
    static unsigned long lastTry = 0;
    const unsigned long RETRY_MS = 3000;
    unsigned long now = millis();
    if (now - lastTry > RETRY_MS) {
      lastTry = now;
      Serial.println("[NET] Attempting STA autoconnect after portal save.");
      WiFi.begin(); // use saved creds from NVS
      lastWifiAttemptMs = millis();

      // close the portal explicitly once creds are in
      if (g_wm.getConfigPortalActive()) stopWiFiPortal();

      g_portalConnectedThisSession = false; // only once
      nextWsAttemptMs = millis() + WS_INITIAL_DELAY_MS;
    }
  }

  // --- WebSocket connect watchdog: bail out if a connect is stuck >5s ---
  if (wsConnecting) {
    const unsigned long WS_CONNECT_STUCK_MS = 1000;
    if (millis() - wsConnectingSinceMs > WS_CONNECT_STUCK_MS) {
      Serial.println("[NET] WS connect watchdog: aborting stuck attempt");
      wsClient.disconnect();
      wsConnected = false;
      wsConnecting = false;
      wsConnectingSinceMs = 0;
      nextWsAttemptMs = millis() + WS_RECONNECT_MS; // backoff before next try
    }
  }

  // If connected to Wi-Fi, maybe resolve and maybe schedule WS connect
  if (WiFi.isConnected()) {

    // --- NEW FIX v3 ---
    // Gate ALL network activity (mDNS, WS) during critical/interactive states
    extern PropState currentState;  // from State.h
    bool inCriticalGameplay =
      (currentState == ARMED) ||
      (currentState == DISARMING_KEYPAD) ||
      (currentState == DISARMING_MANUAL) ||
      (currentState == DISARMING_RFID) ||
      // Also include all interactive states to prevent lag
      (currentState == PROP_IDLE) ||
      (currentState == ARMING) ||
      (currentState == CONFIG_MODE);

    unsigned long now = millis();
    
    // Respect cooldown after repeated failures
    if (wsBackoffUntilMs && now < wsBackoffUntilMs) {
      // Stay quiet until cooldown expires
      if (now + WS_RECONNECT_MS > nextWsAttemptMs) nextWsAttemptMs = now + WS_RECONNECT_MS;
      
    } else if (inCriticalGameplay) {
      // Defer all network activity until we are in a non-interactive state
      if (now + 1500 > nextWsAttemptMs) nextWsAttemptMs = now + 1500;

    } else if (!wsConnected && !wsConnecting && now >= nextWsAttemptMs) {
      // --- THIS IS THE KEY CHANGE ---
      // We are clear to attempt ONE connection.
      // Do all blocking network tasks INSIDE this timed block.
      
      bool canTry = false;
      if (settings.net_use_mdns) {
        // 1. First blocking call: mDNS resolve (0.5s)
        if (resolveScoreboardIP()) {
          canTry = (cachedScoreboardIP != IPAddress(0,0,0,0));
        }
      } else {
        canTry = true; // Static IP, always "try"
      }
      
      if (canTry) {
        // 2. Second blocking call: WebSocket connect (1.0s)
        connectWebSocket();
      }
      
      // 3. Set the backoff timer REGARDLESS of success.
      //    The watchdog will handle the fail-and-retry.
      nextWsAttemptMs = now + WS_RECONNECT_MS;
    }

  } else {
    // Not connected to Wi-Fi: gentle reminder and push WS attempts out
    static unsigned long lastMsg = 0;
    const unsigned long HINT_MS = 10000;
    unsigned long now = millis();
    if (now - lastMsg > HINT_MS) {
      lastMsg = now;
      Serial.println("[NET] Not connected. Use Network->WiFi Setup to configure.");
    }
    if (now + WS_RECONNECT_MS > nextWsAttemptMs) nextWsAttemptMs = now + WS_RECONNECT_MS;
  }

  // NOTE: mDNS.begin() is intentionally NOT called from the loop anymore.

  // WebSocket maintenance
  wsClient.loop();
}
