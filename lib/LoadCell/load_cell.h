#include <Arduino.h>

#define HX711_DOUT 5 // data pin
#define HX711_SCK 6  // clock pin
#define NUM_SAMPLES 100

void hx711_begin(const uint8_t gain=128);
void hx711_tare(uint16_t times=NUM_SAMPLES);
float hx711_get_weight(uint16_t times=NUM_SAMPLES);

