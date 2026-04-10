#include "STP16CPC26.h"
#include <Arduino.h>
#include <SPI.h>

// pinout:
#define LATCH_PIN 34
#define BLANK_PIN 35
#define DATA_PIN 37
#define CLK_PIN 36

// hardware setup:
#define N_LEDS 48

// led data:
uint16_t nodes[N_LEDS];

// create the TLC5947 object:
// The led array needs to be initialized prior to this object creation
STP16CPC26 ledDriver(nodes, N_LEDS, CLK_PIN, DATA_PIN, LATCH_PIN, BLANK_PIN);

// timing:
uint16_t delayTime = 500;

// fade settings
const uint16_t minBrightness = 0;
const uint16_t maxBrightness = 0xFFFF;
const uint32_t fadeTime = 5000; // fadetime in milliseconds
const uint16_t FPS = 100; // frames per second

// fadeVariables
uint16_t frameInterval = 1000 / FPS;
uint32_t lastFrameUpdate = 0;
uint32_t lastFrameStart = 0;

void setup() { Serial.begin(115200); }

void loop()
{
    uint32_t now = millis();
    if (now - lastFrameUpdate >= frameInterval) {
        lastFrameUpdate = now;
        uint16_t brightness = 0;
        uint32_t timeSinceFadeStart = now - lastFrameStart;
        if (timeSinceFadeStart < fadeTime / 2) {
            brightness = map(timeSinceFadeStart, 0, fadeTime / 2, minBrightness, maxBrightness);

        } else if (timeSinceFadeStart < fadeTime) {
            brightness = map(timeSinceFadeStart, fadeTime / 2, fadeTime, maxBrightness, minBrightness);

        } else {
            // fade complete, reset the fade:
            brightness = 0;
            lastFrameStart = now;
        }
        ledDriver.setAllLedsTo(brightness);
        ledDriver.update();
    }
}
