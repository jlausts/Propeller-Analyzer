/**
 *
 * HX711 library for Arduino
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/
#ifndef HX711_h
#define HX711_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class HX711
{
	private:
		byte PD_SCK = 3;	// Power Down and Serial Clock Input Pin
		byte DOUT = 4;		// Serial Data Output Pin
		byte GAIN = 1;		// amplification factor
		long OFFSET = 0;	// used for tare weight
		float SCALE = 2280.f;	// used to return weight in grams, kg, ounces, whatever

	public:

		HX711();

		virtual ~HX711();

		// Initialize library with data output pin, clock input pin and gain factor.
		// Channel selection is made by passing the appropriate gain:
		// - With a gain factor of 64 or 128, channel A is selected
		// - With a gain factor of 32, channel B is selected
		// The library default is "128" (Channel A).
		void begin();

		float get_weight(byte times=1);

		void wait_for_isr();
		
		void wait_ready();

		uint32_t read();

		// returns an average reading; times = how many times to read
		long read_average(byte times = 10);

		// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
		double get_value(byte times = 1);

		// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
		// times = how many readings to do
		float get_units(byte times = 1);

		// set the OFFSET value for tare weight; times = how many times to read the tare value
		void tare(byte times = 80);

		// puts the chip into power down mode
		void power_down();

		// wakes up the chip after power down mode
		void power_up();
};

#endif /* HX711_h */