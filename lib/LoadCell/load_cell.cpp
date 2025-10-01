#include "load_cell.h"



static float HX711_OFFSET = 0;   // determined my tare
static float HX711_SCALE = 1.0f; // change later.
static uint8_t HX711_GAIN = 1;

static PortGroup *port_dout;
static PortGroup *port_sck;
static uint32_t mask_dout;
static uint32_t mask_sck;



inline bool hx711_is_ready()
{
    return (port_dout->IN.reg & mask_dout) == 0;
}

uint32_t hx711_read()
{
    while (!hx711_is_ready());

    uint32_t data = 0;

    for (uint8_t i = 0; i < 24; i++)
    {
        // clock high
        port_sck->OUTSET.reg = mask_sck;
        __asm__ __volatile__("nop\nnop"); // short delay

        // shift in data
        data = (data << 1) | ((port_dout->IN.reg & mask_dout) ? 1 : 0);

        // clock low
        port_sck->OUTCLR.reg = mask_sck;
        __asm__ __volatile__("nop\nnop");
    }

    // gain select (25th..27th pulses)
    for (uint8_t i = 0; i < HX711_GAIN; i++)
    {
        port_sck->OUTSET.reg = mask_sck;
        __asm__ __volatile__("nop\nnop");
        port_sck->OUTCLR.reg = mask_sck;
        __asm__ __volatile__("nop\nnop");
    }

    // sign extend 24-bit to 32-bit
    if (data & 0x800000)
        data |= 0xFF000000;

    return data;
}

void hx711_begin(const uint8_t gain)
{
    pinMode(HX711_DOUT, INPUT);
    pinMode(HX711_SCK, OUTPUT);
    digitalWrite(HX711_SCK, LOW);

    // map pins to fast registers
    port_dout = digitalPinToPort(HX711_DOUT);
    mask_dout = digitalPinToBitMask(HX711_DOUT);
    port_sck = digitalPinToPort(HX711_SCK);
    mask_sck = digitalPinToBitMask(HX711_SCK);

    if (gain == 128)
        HX711_GAIN = 1;
    else if (gain == 64)
        HX711_GAIN = 3;
    else if (gain == 32)
        HX711_GAIN = 2;
    else
        HX711_GAIN = 1;

    // dummy read to apply gain
    (void)hx711_read();
}

float hx711_read_average(uint16_t times)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < times; i++)
        sum += hx711_read();
    return (float)sum / (float)times;
}

void hx711_tare(uint16_t times)
{
    HX711_OFFSET = hx711_read_average(times);
}

float hx711_get_weight(uint16_t times)
{
    return (hx711_read_average(times) - HX711_OFFSET) / HX711_SCALE;
}


