/**
 *
 * HX711 library for Arduino
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
 **/
#include <Arduino.h>
#include "HX711.h"
extern volatile bool in_isr;

HX711::HX711()
{
}

HX711::~HX711()
{
}

void HX711::begin()
{
	pinMode(PD_SCK, OUTPUT);
	pinMode(DOUT, INPUT_PULLUP);
}

void HX711::wait_for_isr()
{
	in_isr = true;
	while(in_isr);
}

uint32_t HX711::read()
{
	wait_ready();
	uint32_t value = 0;
	uint8_t i = 0;
	for (uint8_t k = 0; k < 3; ++k)
	{
		wait_for_isr();

		for (uint8_t j = 0; j < 8; ++i, ++j)
		{
			digitalWrite(PD_SCK, HIGH);
			delayMicroseconds(1);
			value |= digitalRead(DOUT) << (23 - i);
			digitalWrite(PD_SCK, LOW);
			delayMicroseconds(1);
		}
	}

	// Set the channel and the gain factor for the next reading using the clock pin.
	wait_for_isr();
	for (unsigned int i = 0; i < GAIN; i++)
	{
		digitalWrite(PD_SCK, HIGH);
		delayMicroseconds(1);
		digitalWrite(PD_SCK, LOW);
		delayMicroseconds(1);
	}

	return value;
}

void HX711::wait_ready()
{
	while (!(digitalRead(DOUT) == LOW));
}

long HX711::read_average(byte times)
{
	long sum = 0;
	for (byte i = 0; i < times; i++)
	{
		sum += read();
		delay(0);
	}
	return sum / times;
}

double HX711::get_value(byte times)
{
	return read_average(times) - OFFSET;
}

float HX711::get_units(byte times)
{
	return get_value(times) / SCALE;
}

float HX711::get_weight(byte times) 
{
	const float measured = get_units(times);
    return -0.000235f * measured * measured + 2.355f * measured;
}


void HX711::tare(byte times)
{
	double sum = read_average(times);
	OFFSET = sum;
}

void HX711::power_down()
{
	digitalWrite(PD_SCK, LOW);
	digitalWrite(PD_SCK, HIGH);
}

void HX711::power_up()
{
	digitalWrite(PD_SCK, LOW);
}
