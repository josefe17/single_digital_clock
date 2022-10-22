/*
 * NTCSensor.h
 *
 *  Created on: 4 may. 2019
 *      Author: josefe
 */

#ifndef NTCSENSOR_H_
#define NTCSENSOR_H_

#define BETA 3950.0
#define CESIUS2KELVIN_CONSTANT 273.15
#define SERIESRESISTOR_OHMS 10000.0
#define THERMISTORNOMINAL_OHMS 10000.0
#define TEMPERATURENOMINAL (CESIUS2KELVIN_CONSTANT + 25.0)

double readTempCelsius(unsigned char channel);
double readTempKelvin(unsigned char channel);
double readTempFahrenheit(unsigned char channel);

#endif /* NTCSENSOR_H_ */
