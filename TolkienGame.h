// TolkienGame.h
// VERSION: 2.6.1
// Mini-game based on Lord of the Rings trivia.
// Triggered by holding '0' on boot.
// UPDATED: Removed all FastLED.show() calls to prevent flickering.
// UPDATED: Exact color palettes for every Question and Splash screen.
// FIXED: Restored missing tRain helper function.

#pragma once
#include <Arduino.h>
#include "Hardware.h"
#include "State.h"
#include "Display.h"
#include "Utils.h"
#include "Sounds.h"

// --- CONFIGURATION ---
static const int POINTS_PER_Q = 10;
static const int DEFAULT_FEEDBACK_MS = 3000; 
static const int SPLASH_DURATION_MS = 3500;

struct TolkienQuestion {
    const char* category;
    const char* question;
    const char* answer;     
    int rewardTrackId;      
    uint32_t rewardDurationMs; 
    int sectionTrackId;     
};

static const TolkienQuestion TOLKIEN_ROUNDS[] = {
    {"THE RING VERSE", "RINGS FOR ELVES?", "3", 72, 6000, 71},            // Q1 (idx 0)
    {"THE RING VERSE", "RINGS FOR DWARVES?", "7", 73, 6000, 0},           // Q2
    {"THE RING VERSE", "RINGS FOR MEN?", "9", 74, 8000, 0},             // Q3
    {"THE RING VERSE", "#RINGS FOR DARK LORD?", "1", 75, 38000, 0},      // Q4
    {"THE SHIRE", "BILBO AGE @ PARTY?", "111", 77, 7000, 76},            // Q5 (idx 4)
    {"THE SHIRE", "TROLLS @ DAWN?", "3", 78, 21000, 0},                  // Q6
    {"THE SHIRE", "MEALS PER DAY?", "7", 79, 21000, 0},                  // Q7
    {"THE SHIRE", "HOBBITS LEAVING?", "4", 0, 3000, 0},                  // Q8
    {"FELLOWSHIP", "TOTAL MEMBERS?", "9", 80, 12000, 88},                // Q9 (idx 8)
    {"FELLOWSHIP", "ISTARI COUNT?", "5", 0, 3000, 0},                    // Q10
    {"FELLOWSHIP", "BLACK RIDERS?", "9", 81, 24000, 0},                  // Q11
    {"FELLOWSHIP", "LENGTH IN MONTHS?", "6", 0, 3000, 0},                // Q12
    {"TWO TOWERS", "LEGOLAS KILL COUNT?", "42", 0, 3000, 74},            // Q13 (idx 12)
    {"TWO TOWERS", "URUK-HAI ARMY #?", "10000", 0, 3000, 0},             // Q14
    {"TWO TOWERS", "DAYS LOOK EAST?", "5", 82, 8000, 0},                 // Q15
    {"TWO TOWERS", "# ENT DROUGH DRINK?", "2", 0, 3000, 0},               // Q16
    {"RETURN OF KING", "RINGS DESTROYED?", "1", 0, 3000, 85},            // Q17 (idx 16)
    {"RETURN OF KING", "FINGERS LEFT?", "9", 0, 3000, 0},                 // Q18
    {"RETURN OF KING", "FELLOWSHIP DEATHS?", "1", 83, 12000, 0},         // Q19
    {"RETURN OF KING", "SHELOB LEGS?", "8", 0, 3000, 0},                 // Q20
    {"BOOK LORE", "FRODO AGE @ QUEST?", "50", 0, 3000, 89},              // Q21 (idx 20)
    {"BOOK LORE", "YEARS WAITED?", "17", 0, 3000, 0},                    // Q22
    {"BOOK LORE", "KNOWN PALANTIRI?", "7", 84, 14000, 0},                // Q23
    {"BOOK LORE", "HIDDEN ELVEN RNS?", "3", 0, 3000, 0},                 // Q24
    {"HISTORY BUFF", "RING FOUND YEAR?", "2941", 0, 3000, 90},           // Q25 (idx 24)
    {"HISTORY BUFF", "YEAR RING DESTROYED", "3019", 85, 2000, 0},        // Q26
    {"HISTORY BUFF", "ARAGORN'S AGE?", "87", 0, 3000, 0},                // Q27
    {"HISTORY BUFF", "ELENDIL SHIPS?", "9", 0, 3000, 0},                 // Q28
    {"SILMARILLION", "SILMARILS MADE?", "3", 0, 3000, 87},               // Q29 (idx 28)
    {"SILMARILLION", "FEANOR SONS?", "7", 0, 3000, 0},                   // Q30
    {"RAREST LORE", "TOTAL RINGS FORGED?", "20", 86, 14000, 75}          // Q31 (idx 30)
};

static const int TOTAL_TOLKIEN_ROUNDS = sizeof(TOLKIEN_ROUNDS) / sizeof(TOLKIEN_ROUNDS[0]);
enum TolkienState { T_INTRO, T_ROUND_SPLASH, T_QUESTION, T_FEEDBACK, T_GAME_OVER };

static TolkienState tState = T_INTRO;
static int currentTolkienRound = 0;
static int tolkienScore = 0;
static char tolkienInputBuffer[10] = "";
static uint32_t tStateTimer = 0;
static bool tDisplayNeedsUpdate = true;
static bool tLastAnswerCorrect = false;

// --- ANIMATION HELPERS (MODIFIES BUFFER ONLY) ---

inline void add_glitter(fract8 chanceofglitter) {
    if (random8() < chanceofglitter) leds[random16(NUM_LEDS)] += CRGB::White;
}

inline void tTwinkle(CRGB c, uint8_t density = 20, uint8_t fade = 5) {
    fadeToBlackBy(leds, NUM_LEDS, fade);
    if (random8() < density) leds[random16(NUM_LEDS)] = c;
}

inline void tChase(CRGB c1, CRGB c2 = CRGB::Black, uint8_t speed = 100) {
    fadeToBlackBy(leds, NUM_LEDS, 30);
    uint16_t pos = (millis() / speed) % NUM_LEDS;
    leds[pos] = c1;
    if (c2 != CRGB::Black) leds[(pos + 2) % NUM_LEDS] = c2;
}

inline void tWave(CRGB c, uint8_t speed = 15) {
    uint8_t beat = beatsin8(speed, 30, 200);
    for(int i=0; i<NUM_LEDS; i++) {
        uint8_t angle = (i * 256 / NUM_LEDS) + beat;
        leds[i] = c;
        leds[i].nscale8(sin8(angle));
    }
}

inline void tFire() {
    static byte heat[NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++) heat[i] = qsub8(heat[i], random8(0, ((50 * 10) / NUM_LEDS) + 2));
    for (int k = NUM_LEDS - 1; k >= 2; k--) heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    if (random8() < 160) heat[random8(5)] = qadd8(heat[random8(5)], random8(160, 255));
    for (int j = 0; j < NUM_LEDS; j++) leds[j] = HeatColor(heat[j]);
}

inline void tRain(CRGB c) {
    fadeToBlackBy(leds, NUM_LEDS, 30);
    if (random8() < 30) leds[0] = c;
    for (int i = NUM_LEDS - 1; i > 0; i--) leds[i] = leds[i-1];
}

inline void tWizardChase() {
    fadeToBlackBy(leds, NUM_LEDS, 25);
    uint16_t pos = (millis() / 120) % NUM_LEDS;
    leds[pos] = CRGB::White; // Gandalf
    leds[(pos + 3) % NUM_LEDS] = CRGB(160, 80, 0); // Radagast
    leds[(pos + 6) % NUM_LEDS] = CRGB::Blue; // Blue Wizards
}

// --- MAIN LIGHTING ENGINE ---
inline void tolkienUpdateLeds() {
    if (tState == T_FEEDBACK) {
        fill_solid(leds, NUM_LEDS, tLastAnswerCorrect ? CRGB::Green : CRGB::Red);
        return;
    }
    if (tState == T_GAME_OVER) {
        static uint8_t hue = 0;
        fill_rainbow(leds, NUM_LEDS, hue++, 7);
        return;
    }
    if (tState == T_INTRO) {
        tTwinkle(CRGB::Green, 10, 2); // Slow green twinkles
        return;
    }

    bool isSplash = (tState == T_ROUND_SPLASH);
    int r = currentTolkienRound;

    if (isSplash) {
        switch(r) {
            case 0: tChase(CRGB::Green, CRGB::Aqua); break;      // Splash 1
            case 4: tWave(CRGB::Green); break;                  // Splash 2
            case 8: tChase(CRGB::Yellow, CRGB::Green); break;   // Splash 3
            case 12: tChase(CRGB::Red); break;                  // Splash 4
            case 16: fill_solid(leds, NUM_LEDS, CRGB(100, 80, 0)); add_glitter(60); break; // Splash 5
            case 20: fill_solid(leds, NUM_LEDS, CRGB(255, 255, 100)); break; // Splash 6
            case 24: tTwinkle(CRGB(255, 120, 20), 40, 4); break; // Splash 7 (Candle)
            case 28: fill_solid(leds, NUM_LEDS, CRGB::SkyBlue); break; // Splash 8
            case 30: tFire(); break;                            // Splash 9
            default: fill_solid(leds, NUM_LEDS, CRGB::Black); break;
        }
    } else {
        switch(r) {
            case 0: fill_solid(leds, NUM_LEDS, CRGB::Green); break;   // Q1
            case 1: fill_solid(leds, NUM_LEDS, CRGB::Red); break;     // Q2
            case 2: fill_solid(leds, NUM_LEDS, CRGB::SkyBlue); break; // Q3
            case 3: tChase(CRGB::Red, CRGB::Orange, 70); break;       // Q4
            case 4: tWave(CRGB::Green); break;                        // Q5
            case 5: fill_solid(leds, NUM_LEDS, CRGB::Silver); break;  // Q6
            case 6: tChase(CRGB::Green); break;                       // Q7
            case 7: tChase(CRGB::Green); break;                       // Q8
            case 8: tTwinkle(CRGB::Green, 40, 8); break;              // Q9
            case 9: tTwinkle(CRGB::White, 40, 8); break;              // Q10
            case 10: tWave(CRGB::Purple); break;                      // Q11
            case 11: fadeToBlackBy(leds, NUM_LEDS, 2); break;         // Q12 Fade
            case 12: tTwinkle(CRGB::Red, 100, 20); break;             // Q13 Sparks
            case 13: if ((millis()/150)%2) fill_solid(leds, NUM_LEDS, CRGB::Red); else fill_solid(leds, NUM_LEDS, CRGB::White); break; // Q14
            case 14: { uint8_t b = map(millis()-tStateTimer, 0, 4000, 0, 255); fill_solid(leds, NUM_LEDS, CRGB(b, b, 0)); } break; // Q15
            case 15: tChase(CRGB::Green); break;                       // Q16
            case 16: fill_solid(leds, NUM_LEDS, CRGB(100, 80, 0)); add_glitter(60); break; // Q17
            case 17: tChase(CRGB::Orange); break;                      // Q18
            case 18: tRain(CRGB::Blue); break;                         // Q19
            case 19: tChase(CRGB::Purple); break;                      // Q20
            case 20: fill_solid(leds, NUM_LEDS, CRGB::Green); break;   // Q21
            case 21: fill_solid(leds, NUM_LEDS, CRGB::Green); break;   // Q22
            case 22: tWave(CRGB::Purple, 5); break;                    // Q23 Swirl (Purple/Blue)
            case 23: fill_solid(leds, NUM_LEDS, CRGB::Teal); break;    // Q24
            case 24: fill_solid(leds, NUM_LEDS, CRGB::Green); break;   // Q25
            case 25: fill_solid(leds, NUM_LEDS, CRGB::Green); break;   // Q26
            case 26: tTwinkle(CRGB::Blue); break;                      // Q27
            case 27: tTwinkle(CRGB::Green, 30, 4); tTwinkle(CRGB::White, 15, 4); break; // Q28
            case 28: { uint8_t b = map(millis()-tStateTimer, 0, 4000, 0, 255); fill_solid(leds, NUM_LEDS, CRGB(b, b, 0)); } break; // Q29
            case 29: tWizardChase(); break;                            // Q30
            case 30: tFire(); break;                                   // Q31
            default: fill_solid(leds, NUM_LEDS, CRGB::Black); break;
        }
    }
}

inline void updateTolkienLCD() {
    if (!tDisplayNeedsUpdate) return;
    tDisplayNeedsUpdate = false;
    lcd.clear();
    switch (tState) {
        case T_INTRO:
            centerPrintC("LORD OF THE RINGS", 0);
            centerPrintC("TRIVIA CHALLENGE", 1);
            centerPrintC("PRESS # TO BEGIN", 3);
            break;
        case T_ROUND_SPLASH: {
            const TolkienQuestion &q = TOLKIEN_ROUNDS[currentTolkienRound];
            centerPrintC("--- NEW ROUND ---", 0);
            centerPrintC(q.category, 2);
            break;
        }
        case T_QUESTION: {
            const TolkienQuestion &q = TOLKIEN_ROUNDS[currentTolkienRound];
            char header[21];
            snprintf(header, sizeof(header), "Q%d/%d  PTS:%d", currentTolkienRound + 1, TOTAL_TOLKIEN_ROUNDS, tolkienScore);
            centerPrintC(header, 0);
            centerPrintC(q.category, 1);
            centerPrintC(q.question, 2);
            char inputLine[21];
            snprintf(inputLine, sizeof(inputLine), "Input: %s_", tolkienInputBuffer);
            centerPrintC(inputLine, 3);
            break;
        }
        case T_FEEDBACK: {
             if (tLastAnswerCorrect) {
                 centerPrintC("CORRECT!", 1);
                 centerPrintC("+10 POINTS", 2);
             } else {
                 centerPrintC("INCORRECT", 1);
                 const TolkienQuestion &q = TOLKIEN_ROUNDS[currentTolkienRound];
                 char ansBuf[21];
                 snprintf(ansBuf, sizeof(ansBuf), "Ans: %s", q.answer);
                 centerPrintC(ansBuf, 3);
             }
             break;
        }
        case T_GAME_OVER:
            centerPrintC("QUEST COMPLETE", 0);
            char scoreBuf[21];
            snprintf(scoreBuf, sizeof(scoreBuf), "FINAL SCORE: %d", tolkienScore);
            centerPrintC(scoreBuf, 1);
            if (tolkienScore >= (TOTAL_TOLKIEN_ROUNDS * 10)) centerPrintC("RANK: ELESSAR", 3);
            else if (tolkienScore > (TOTAL_TOLKIEN_ROUNDS * 7)) centerPrintC("RANK: WIZARD", 3);
            else if (tolkienScore > (TOTAL_TOLKIEN_ROUNDS * 4)) centerPrintC("RANK: RANGER", 3);
            else centerPrintC("RANK: ORC BAIT", 3);
            break;
    }
}

inline void checkNextRoundLogic() {
    bool showSplash = (currentTolkienRound == 0) || 
                      (strcmp(TOLKIEN_ROUNDS[currentTolkienRound - 1].category, TOLKIEN_ROUNDS[currentTolkienRound].category) != 0);
    if (showSplash) {
        tState = T_ROUND_SPLASH; tStateTimer = millis(); tDisplayNeedsUpdate = true;
        if (TOLKIEN_ROUNDS[currentTolkienRound].sectionTrackId > 0) safePlayForce(TOLKIEN_ROUNDS[currentTolkienRound].sectionTrackId);
    } else {
        tState = T_QUESTION; tDisplayNeedsUpdate = true;
    }
}

inline void startTolkienGame() {
    setState(TOLKIEN_GAME); 
    currentTolkienRound = 0; tolkienScore = 0; tolkienInputBuffer[0] = '\0';
    tDisplayNeedsUpdate = true; tState = T_INTRO; tStateTimer = millis();
    safePlayForce(70); 
}

inline void serviceTolkienGame(char key) {
    uint32_t now = millis();

    // UPDATE HARDWARE (Buffer only, actual .show() is in main sketch loop)
    updateTolkienLCD();
    tolkienUpdateLeds();

    switch (tState) {
        case T_INTRO:
            if (key == '#') checkNextRoundLogic();
            break;
        case T_ROUND_SPLASH:
            if (now - tStateTimer > SPLASH_DURATION_MS) { tState = T_QUESTION; tDisplayNeedsUpdate = true; }
            break;
        case T_FEEDBACK: {
            uint32_t waitTime = tLastAnswerCorrect ? TOLKIEN_ROUNDS[currentTolkienRound].rewardDurationMs : DEFAULT_FEEDBACK_MS;
            if (now - tStateTimer > waitTime) {
                currentTolkienRound++;
                if (currentTolkienRound >= TOTAL_TOLKIEN_ROUNDS) {
                    tState = T_GAME_OVER; tDisplayNeedsUpdate = true; safePlayForce(91);
                } else { checkNextRoundLogic(); }
            }
            break;
        }
        case T_GAME_OVER:
            if (key == '#') setState(STANDBY);
            break;
        case T_QUESTION:
            if (key) {
                tDisplayNeedsUpdate = true;
                if (isdigit(key)) {
                    int len = strlen(tolkienInputBuffer);
                    if (len < 8) { tolkienInputBuffer[len] = key; tolkienInputBuffer[len+1] = '\0'; }
                }
                else if (key == '*') {
                    int len = strlen(tolkienInputBuffer);
                    if (len > 0) tolkienInputBuffer[len-1] = '\0';
                }
                else if (key == '#') {
                    const TolkienQuestion &q = TOLKIEN_ROUNDS[currentTolkienRound];
                    safeStop(); delay(50);
                    if (strcmp(tolkienInputBuffer, q.answer) == 0) {
                        tLastAnswerCorrect = true; tolkienScore += POINTS_PER_Q;
                        if (q.rewardTrackId > 0) safePlayForce(q.rewardTrackId);
                    } else {
                        tLastAnswerCorrect = false; safePlayForce(SOUND_MENU_CANCEL);
                    }
                    tState = T_FEEDBACK; tStateTimer = millis(); tolkienInputBuffer[0] = '\0';
                }
            }
            break;
    }
}