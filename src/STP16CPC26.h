#pragma once
#include <16bitPixelTypes.h>
#include <Arduino.h>

#define BYTESPERDRIVER 32
#define LEDDOTSPERDRIVER 16

#define DEFAULT_SPI_CLK 10000000  // 10MHz

class STP16CPC26 {
 private:
  uint16_t* leds;
  uint16_t nLedDots;
  uint8_t clkPin;
  uint8_t dataPin;
  uint8_t latchPin;
  int8_t blankPin;
  uint32_t clkFrequency;
  uint8_t nLedDrivers;

  void init();

 public:
  // constructors:
  STP16CPC26(RGBColor16* _rgbLedData, uint16_t _nRGBLeds, uint8_t _clkPin,
          uint8_t _dataPin, uint8_t _latchPin, int8_t _blankPin = -1,
          uint32_t _clkFrequency = DEFAULT_SPI_CLK);
  STP16CPC26(RGBWColor16* _rgbwLedData, uint16_t _nRGBWLeds, uint8_t _clkPin,
          uint8_t _dataPin, uint8_t _latchPin, int8_t _blankPin = -1,
          uint32_t _clkFrequency = DEFAULT_SPI_CLK);
  STP16CPC26(uint16_t* _ledData, uint16_t _nLedDots, uint8_t _clkPin,
          uint8_t _dataPin, uint8_t _latchPin, int8_t _blankPin = -1,
          uint32_t _clkFrequency = DEFAULT_SPI_CLK);

  // setting led:
  void setLedTo(uint16_t ledIndex, struct RGBWColor16 color);
  void setLedTo(uint16_t ledIndex, struct RGBColor16 color);
  void setLedTo(uint16_t ledIndex, uint16_t brightness);

  // setting all leds:
  void setAllLedsTo(struct RGBWColor16 color);
  void setAllLedsTo(struct RGBColor16 color);
  void setAllLedsTo(uint16_t brightness);

  // clear all leds:
  void clearLeds();

  // write the ledData to the led drivers:
  void update();
};