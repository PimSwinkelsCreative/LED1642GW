#include <Arduino.h>
#include <SPI.h>
#include <TLC5947.h>

// pinout:
#define LATCH_PIN 15
#define BLANK_PIN 2
#define DATA_PIN 13
#define CLK_PIN 14

// hardware setup:
#define N_LEDS 24

// led data:
RGBWColor16 leds[N_LEDS];

// create the TLC5947 object:
// The led array needs to be initialized prior to this object creation
TLC5947 ledDriver(leds, N_LEDS, CLK_PIN, DATA_PIN, LATCH_PIN, BLANK_PIN);

// timing:
uint16_t delayTime = 500;

void setup() { Serial.begin(115200); }

void loop() {
  ledDriver.setAllLedsTo(RGBWColor16(1000, 0, 0, 0));
  ledDriver.update();
  delay(delayTime);
  ledDriver.setAllLedsTo(RGBWColor16(0, 1000, 0, 0));
  ledDriver.update();
  delay(delayTime);
  ledDriver.setAllLedsTo(RGBWColor16(0, 0, 1000, 0));
  ledDriver.update();
  delay(delayTime);
  ledDriver.setAllLedsTo(RGBWColor16(0, 0, 0, 1000));
  ledDriver.update();
  delay(delayTime);

  // ledDriver.setAllLedsTo({10,10,10,10});
  // ledDriver.update();
  // delayMicroseconds(10);
}
