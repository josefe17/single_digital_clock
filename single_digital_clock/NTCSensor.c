/*
 * NTCSensor.c
 *
 *  Created on: 4 may. 2019
 *      Author: josefe
 */

#include "NTCSensor.h"
#include <math.h>
#include "ADCDriver.h"

double readTempKelvin(unsigned char channel)
{
	double raw = (double) readADC(channel);
	raw = (ADC_RESOLUTION-1) / raw -1;
	double resistance = SERIESRESISTOR_OHMS / raw;
	double steinhart = resistance / THERMISTORNOMINAL_OHMS;     // (R/Ro)
	steinhart = log(steinhart);                  // ln(R/Ro)
	steinhart /= BETA;                   // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL); // + (1/To)
	steinhart = 1.0 / steinhart;                 // Invert
	return steinhart;
}

double readTempCelsius(unsigned char channel)
{
	return readTempKelvin(channel) - CESIUS2KELVIN_CONSTANT;
}
