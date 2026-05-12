#pragma once
#include <16bitPixelTypes.h>
#include <Arduino.h>
#include <driver/rmt_tx.h> //required fot RMT control

#define LEDDOTSPERDRIVER 16
#define BYTESPERDRIVER (2 * LEDDOTSPERDRIVER)

#define MAXBRIGHTNESS 0x3F

#define MAX_DRIVER_AMOUNT 1
#define MAX_RMT_MESSAGE_LENGTH (MAX_DRIVER_AMOUNT * 8 * BYTESPERDRIVER)

#define RMT_CLOCK_HZ 2000000 // 2MHz

class LED1642GW {
private:
    uint16_t* leds;
    uint16_t nLedDots;
    uint8_t clkPin;
    uint8_t dataPin;
    uint8_t latchPin;
    int8_t pwmClockPin;
    uint32_t clkFrequency;
    uint8_t nLedDrivers;

    uint32_t clkPinBitmap;
    uint32_t dataPinBitmap;
    uint32_t latchPinBitmap;

    uint64_t lastSettingsUpdate;
    uint32_t settingUpdateInterval;

    uint8_t brightness;

    rmt_channel_handle_t clkRMTChannel;
    rmt_channel_handle_t dataRMTChannel;
    rmt_channel_handle_t latchRMTChannel;

    rmt_sync_manager_t *ledDriverRmtSyncManager;
    rmt_encoder_handle_t copy_encoder;

    void init();
    void setConfigRegister();
    void enableOutputs();
    void startPWMClock();

    void pulseClock();
    void setDataPin(bool value);
    void setLatchPin();
    void clearLatchPin();

public:
    // constructors:
    LED1642GW(RGBColor16* _rgbLedData, uint16_t _nRGBLeds, uint8_t _clkPin,
        uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin = -1);
    LED1642GW(RGBWColor16* _rgbwLedData, uint16_t _nRGBWLeds, uint8_t _clkPin,
        uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin = -1);
    LED1642GW(uint16_t* _ledData, uint16_t _nLedDots, uint8_t _clkPin,
        uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin = -1);

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

    // function that sets the interval at which the driver config is being sent
    void setConfigUpdateInterval(uint32_t milliseconds);

    void setBrightness(uint8_t _brightness);
};