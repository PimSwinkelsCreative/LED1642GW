# LED1642GW
An Arduino compatible LED1642GW LED driver library for ESP32 that uses direct register manipulation of the output registers to generate the required Clk, Data and Latch signals.

The Library is blocking during LED update. Updating one 16 output driver takes approximately 82 microseconds when the ESP runs on 240MHz.

The driver requires a config register and an output enable register to be set correctly in order for the LEDs to be powered. To allow for asynchronous power-up, the controller periodically sends these settings along with the LED data.

If the Drivers do not have a hardware PWM Clock connected, the Library can provide a 10MHz PWM reference clock using the built-in I2S hardware. This does however limit the PWM frequency to just 150Hz in 16-bit mode. It is advised to use a higher frequency hardware solution.
