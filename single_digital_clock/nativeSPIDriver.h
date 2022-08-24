/*
 * nativeSPIDriver.h
 *
 * Created: 24/08/2022 21:27:36
 *  Author: josefe
 */ 

#include <avr/io.h>

#define DDR_SPI DDRB
#define DD_MOSI DDB3
#define DD_SCK DDB5

void nativeSPIDriverInit();
void nativeSPIDriverWrite(unsigned char byte);

