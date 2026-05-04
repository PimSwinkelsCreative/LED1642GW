#include "LED1642GW.h"
#include <driver/gpio.h> //used to directly set the I/O registers
#include <driver/i2s.h> //used fot the 10MHz I2S clock

#include <soc/gpio_reg.h> //used to directly set the I/O registers
#include <soc/gpio_struct.h> //used to directly set the I/O registers

#define PULSEDURATION 1

LED1642GW::LED1642GW(uint16_t* _ledData, uint16_t _nLedDots, uint8_t _clkPin,
    uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin)
    : leds(_ledData)
    , nLedDots(_nLedDots)
    , clkPin(_clkPin)
    , dataPin(_dataPin)
    , latchPin(_latchPin)
    , pwmClockPin(_pwmClockPin)
{
    init();
}

LED1642GW::LED1642GW(RGBColor16* _rgbLedData, uint16_t _nRGBLeds, uint8_t _clkPin,
    uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin)
    : leds((uint16_t*)_rgbLedData)
    , nLedDots(_nRGBLeds * (sizeof(_rgbLedData[0]) / sizeof(_rgbLedData[0].r)))
    , clkPin(_clkPin)
    , dataPin(_dataPin)
    , latchPin(_latchPin)
    , pwmClockPin(_pwmClockPin)
{
    init();
}

LED1642GW::LED1642GW(RGBWColor16* _rgbwData, uint16_t _nRGBWLeds, uint8_t _clkPin,
    uint8_t _dataPin, uint8_t _latchPin, int8_t _pwmClockPin)
    : leds((uint16_t*)_rgbwData)
    , nLedDots(_nRGBWLeds * (sizeof(_rgbwData[0]) / sizeof(_rgbwData[0].r)))
    , clkPin(_clkPin)
    , dataPin(_dataPin)
    , latchPin(_latchPin)
    , pwmClockPin(_pwmClockPin)
{
    init();
}

void LED1642GW::init()
{

    Serial.println("LED1642GW::init() started\n");
    nLedDrivers = nLedDots / LEDDOTSPERDRIVER;
    if (nLedDots % LEDDOTSPERDRIVER > 0) {
        nLedDrivers++;
    }

    pinMode(clkPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    // digitalWrite(latchPin, LOW);

    clkPinBitmap = 1 << (clkPin % 32);
    dataPinBitmap = 1 << (dataPin % 32);
    latchPinBitmap = 1 << (latchPin % 32);

    lastSettingsUpdate = 0;
    settingUpdateInterval = 1000;

    brightness = MAXBRIGHTNESS;

    // setup the RMT interface

    // clk signal
    // config.gpio_num = gpio_num_t(clkPin);
    rmt_tx_channel_config_t chan_config_clk = {
        .gpio_num = GPIO_NUM_36,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_CLOCK_HZ,
        .mem_block_symbols = 48, // CHECK THIS!
        .trans_queue_depth = 1, // only 1 message at the same time  >> UPDATE IF PWM CLOCK IS RMT BASED AS WELL!
        .intr_priority = 0
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&chan_config_clk, &clkRMTChannel));
    ESP_ERROR_CHECK(rmt_enable(clkRMTChannel));

    // data signal:
    // config.gpio_num = intToGpio(dataPin);
    rmt_tx_channel_config_t chan_config_dat = {
        .gpio_num = GPIO_NUM_37,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_CLOCK_HZ,
        .mem_block_symbols = 48, // CHECK THIS!
        .trans_queue_depth = 1, // only 1 message at the same time  >> UPDATE IF PWM CLOCK IS RMT BASED AS WELL!
        .intr_priority = 0
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&chan_config_dat, &dataRMTChannel));
    ESP_ERROR_CHECK(rmt_enable(dataRMTChannel));

    // latch signal:
    // config.gpio_num = intToGpio(latchPin);
    rmt_tx_channel_config_t chan_config_latch = {
        .gpio_num = GPIO_NUM_34,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_CLOCK_HZ,
        .mem_block_symbols = 48, // CHECK THIS!
        .trans_queue_depth = 1, // only 1 message at the same time  >> UPDATE IF PWM CLOCK IS RMT BASED AS WELL!
        .intr_priority = 0
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&chan_config_latch, &latchRMTChannel));
    ESP_ERROR_CHECK(rmt_enable(latchRMTChannel));

    // make sure the channels are synced:
    rmt_channel_handle_t rmtChannels[] = { clkRMTChannel, dataRMTChannel, latchRMTChannel };
    rmt_sync_manager_config_t rms_sync_config = { rmtChannels, 3 };

    rmt_new_sync_manager(&rms_sync_config, &ledDriverRmtSyncManager);

    // Create and use a simple encoder
    rmt_copy_encoder_config_t copy_encoder_config = { };
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder));

    // setConfigRegister();
    // enableOutputs();

    if (pwmClockPin >= 0) {
        startPWMClock();
    }

    Serial.println("Reached end of INIT!\n");
}

void LED1642GW::setConfigRegister()
{
    // build the config register:
    uint16_t cfg = 0;
    cfg |= brightness; // CFG0..5 = max gain
    cfg |= (1 << 6); // CFG6 = high current range
    cfg |= (1 << 7); // CFG7 = normal mode
    cfg |= (1 << 13); // CFG13 = SDO delay enable

    for (int driver = nLedDrivers - 1; driver >= 0; driver--) {
        for (int i = 15; i >= 0; i--) {
            setDataPin((cfg & 0x01 << i) >> i);

            if (driver == 0) {
                if (i == 6) {
                    setLatchPin();
                }
            }
            pulseClock();
        }
        clearLatchPin();
    }
    setDataPin(false);

    Serial.println("Config Registers set!\n");
}

void LED1642GW::enableOutputs()
{
    for (int driver = nLedDrivers - 1; driver >= 0; driver--) {
        for (int i = 15; i >= 0; i--) {
            setDataPin(true); // all pins need to be enabled, so data is always "1"

            if (driver == 0) {
                if (i == 1) {
                    digitalWrite(latchPin, HIGH);
                }
            }
            pulseClock();
        }
        clearLatchPin();
    }
    setDataPin(false);

    Serial.println("outputs enabled!\n");
}

void LED1642GW::startPWMClock()
{
    // use the I2S clock as the 10MHz PWMclock:
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 625000, // clock is 16 times this value, so 10MHz
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 2,
        .dma_buf_len = 8,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = pwmClockPin,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    Serial.println("PWM Clock Started!\n");
}

void LED1642GW::update()
{
    Serial.println("Starting the Update function\n");

    rmt_tx_wait_all_done(clkRMTChannel, portMAX_DELAY);
    rmt_tx_wait_all_done(dataRMTChannel, portMAX_DELAY);
    rmt_tx_wait_all_done(latchRMTChannel, portMAX_DELAY);

    // periodically re-send the config data to allow for delayed power on of the interconnect boards:
    if (millis() - lastSettingsUpdate > settingUpdateInterval) {
        // rmt_tx_wait_all_done(clkRMTChannel, portMAX_DELAY); // wait for the RMT to be done
        lastSettingsUpdate = millis();
        setConfigRegister();
        enableOutputs();
    }

    // create the led data message:
    // rmt_symbol_word_t clkData[MAX_RMT_MESSAGE_LENGTH];
    rmt_symbol_word_t* clkData = (rmt_symbol_word_t*)heap_caps_malloc(
        sizeof(rmt_symbol_word_t) * MAX_RMT_MESSAGE_LENGTH,
        MALLOC_CAP_DMA);
    // rmt_symbol_word_t dataData[MAX_RMT_MESSAGE_LENGTH];
    rmt_symbol_word_t* dataData = (rmt_symbol_word_t*)heap_caps_malloc(
        sizeof(rmt_symbol_word_t) * MAX_RMT_MESSAGE_LENGTH,
        MALLOC_CAP_DMA);
    // rmt_symbol_word_t latchData[MAX_RMT_MESSAGE_LENGTH];
    rmt_symbol_word_t* latchData = (rmt_symbol_word_t*)heap_caps_malloc(
        sizeof(rmt_symbol_word_t) * MAX_RMT_MESSAGE_LENGTH,
        MALLOC_CAP_DMA);

    Serial.println("created the rmt data arrays\n");

    uint32_t messageIndex = 0;
    for (int channel = 15; channel >= 0; channel--) {
        for (int driver = nLedDrivers - 1; driver >= 0; driver--) {
            uint16_t nodeIndex = driver * LEDDOTSPERDRIVER + channel;
            for (int i = 15; i >= 0; i--) {
                // setDataPin((leds[nodeIndex] & 0x01 << i) >> i);
                // dataData[messageIndex] = { { 2, uint8_t((leds[nodeIndex] & 0x01 << i) >> i), 0, 0 } }; // set the whole clock period to the required level
                dataData[messageIndex].duration0 = PULSEDURATION;
                dataData[messageIndex].level0 = (leds[nodeIndex] & 0x01 << i) >> i ? 1 : 0;
                dataData[messageIndex].duration1 = PULSEDURATION;
                dataData[messageIndex].level1 = (leds[nodeIndex] & 0x01 << i) >> i ? 1 : 0;
                if (driver == 0) {
                    if (channel > 0) {
                        if (i <= 3) {
                            // setLatchPin();
                            // latchData[messageIndex] = { { 2, 1, 0, 0 } };
                            latchData[messageIndex].duration0 = PULSEDURATION;
                            latchData[messageIndex].level0 = 1;
                            latchData[messageIndex].duration1 = PULSEDURATION;
                            latchData[messageIndex].level1 = 1;
                        }
                    } else {
                        if (i <= 5) {
                            // setLatchPin();
                            // latchData[messageIndex] = { { 2, 1, 0, 0 } };
                            latchData[messageIndex].duration0 = PULSEDURATION;
                            latchData[messageIndex].level0 = 1;
                            latchData[messageIndex].duration1 = PULSEDURATION;
                            latchData[messageIndex].level1 = 1;
                        }
                    }
                } else {
                    // latchData[messageIndex] = { { 2, 0, 0, 0 } };
                    latchData[messageIndex].duration0 = PULSEDURATION;
                    latchData[messageIndex].level0 = 0;
                    latchData[messageIndex].duration1 = PULSEDURATION;
                    latchData[messageIndex].level1 = 0;
                }
                // pulseClock();
                // clkData[messageIndex] = { { 1, 0, 1, 1 } };
                clkData[messageIndex].duration0 = PULSEDURATION;
                clkData[messageIndex].level0 = 0;
                clkData[messageIndex].duration1 = PULSEDURATION;
                clkData[messageIndex].level1 = 1;
                messageIndex++;
            }
            // clearLatchPin();
        }
    }

    for (int i = 0; i < messageIndex; i++) {
        if (clkData[i].duration0 == 0 || clkData[i].duration1 == 0 || latchData[i].duration0 == 0 || latchData[i].duration1 == 0 || dataData[i].duration0 == 0 || dataData[i].duration1 == 0) {
            Serial.println("ERROR: Duration set to 0!");
        }
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // Set to -1 for endless loop
    };

    Serial.println("filled the rmt data arrays\n");
    Serial.println(messageIndex);

    ESP_ERROR_CHECK(rmt_transmit(clkRMTChannel, copy_encoder, clkData, sizeof(rmt_symbol_word_t) * messageIndex, &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(dataRMTChannel, copy_encoder, dataData, sizeof(rmt_symbol_word_t) * messageIndex, &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(latchRMTChannel, copy_encoder, latchData, sizeof(rmt_symbol_word_t) * messageIndex, &tx_config));
    ESP_ERROR_CHECK(rmt_sync_reset(ledDriverRmtSyncManager));
    // ESP_ERROR_CHECK(rmt_sync_start(ledDriverRmtSyncManager));

    // setDataPin(false);

    Serial.println("sent the rmt data arrays\n");
}

void LED1642GW::setLedTo(uint16_t ledIndex, struct RGBWColor16 color)
{
    ledIndex = ledIndex * sizeof(color) / sizeof(color.r);
    if (ledIndex >= nLedDots)
        return;
    memcpy(&leds[ledIndex], &color, sizeof(color));
}

void LED1642GW::setLedTo(uint16_t ledIndex, struct RGBColor16 color)
{
    if (ledIndex >= nLedDots * sizeof(color) / sizeof(color.r))
        return;
    memcpy(&leds[ledIndex], &color, sizeof(color));
}

void LED1642GW::setLedTo(uint16_t ledIndex, uint16_t brightness)
{
    if (ledIndex >= nLedDots)
        return; // catch out of bounds index
    leds[ledIndex] = brightness;
}

void LED1642GW::setAllLedsTo(struct RGBWColor16 color)
{
    for (int i = 0; i < nLedDots; i++) {
        switch (i % 4) {
        case 0:
            leds[i] = color.r;
            break;
        case 1:
            leds[i] = color.g;
            break;
        case 2:
            leds[i] = color.b;
            break;
        case 3:
            leds[i] = color.w;
            break;
        default:
            leds[i] = 0; // should not occur
            break;
        }
    }
}
void LED1642GW::setAllLedsTo(struct RGBColor16 color)
{
    for (int i = 0; i < nLedDots; i++) {
        switch (i % 3) {
        case 0:
            leds[i] = color.r;
            break;
        case 1:
            leds[i] = color.g;
            break;
        case 2:
            leds[i] = color.b;
            break;
        default:
            leds[i] = 0; // should not occur
            break;
        }
    }
}
void LED1642GW::setAllLedsTo(uint16_t brightness)
{
    for (int i = 0; i < nLedDots; i++) {
        leds[i] = brightness;
    }
}

void LED1642GW::clearLeds()
{
    for (int i = 0; i < nLedDots; i++) {
        leds[i] = 0;
    }
}

void LED1642GW::setBrightness(uint8_t _brightness)
{
    brightness = constrain(_brightness, 0, MAXBRIGHTNESS);
    setConfigRegister();
}

void LED1642GW::pulseClock()
{
    if (clkPin < 31) {
        GPIO.out_w1ts = clkPinBitmap; // set pin
        GPIO.out_w1tc = clkPinBitmap; // clear pin
    } else {
        GPIO.out1_w1ts.val = clkPinBitmap; // set pin
        GPIO.out1_w1tc.val = clkPinBitmap; // clear pin
    }
}
void LED1642GW::setDataPin(bool value)
{
    if (dataPin < 31) {
        if (value) {
            GPIO.out_w1ts = dataPinBitmap;
        } else {
            GPIO.out_w1tc = dataPinBitmap;
        }
    } else {
        if (value) {
            GPIO.out1_w1ts.val = dataPinBitmap;
        } else {
            GPIO.out1_w1tc.val = dataPinBitmap;
        }
    }
}
void LED1642GW::setLatchPin()
{
    if (latchPin < 31) {
        GPIO.out_w1ts = latchPinBitmap;
    } else {
        GPIO.out1_w1ts.val = latchPinBitmap;
    }
}
void LED1642GW::clearLatchPin()
{
    if (latchPin < 31) {
        GPIO.out_w1tc = latchPinBitmap;
    } else {
        GPIO.out1_w1tc.val = latchPinBitmap;
    }
}

void LED1642GW::setConfigUpdateInterval(uint32_t milliseconds)
{
    settingUpdateInterval = milliseconds;
}