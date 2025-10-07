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

#include "Arduino.h"
const int LOADCELL_DOUT_PIN = 3;
const int LOADCELL_SCK_PIN = 4;
class HX711
{
	private:
		byte PD_SCK = 4;	   // 3, Power Down and Serial Clock Input Pin
		byte DOUT   = 3;	   // 4, Serial Data Output Pin
		byte GAIN   = 1;	   // 1, amplification factor
		long OFFSET = 0;	   // used for tare weight
		float SCALE = 2280.f;  // used to return weight in grams, kg, ounces, whatever
		const bool invert = true;

	public:

		HX711();

		virtual ~HX711();

		// Initialize library with data output pin, clock input pin and gain factor.
		// Channel selection is made by passing the appropriate gain:
		// - With a gain factor of 64 or 128, channel A is selected
		// - With a gain factor of 32, channel B is selected
		// The library default is "128" (Channel A).
		void begin();

		float get_weight(byte times=50);

		void wait_ready();

		uint32_t read();

		// returns an average reading; times = how many times to read
		long read_average(byte times = 10);

		// set the OFFSET value for tare weight; times = how many times to read the tare value
		void tare(byte times = 100);
};

#endif /* HX711_h */