/*
 * USARTSPIDriver.h
 *
 * Created: 24/08/2022 21:39:36
 *  Author: josefe
 */ 

#include <avr/io.h>

#define XCK0_DDR DDRD
#define XCK0 DDD4
#define USARTSPIBAUD 7 

void USARTSPIDriverInitTXOnly();
void USARTSPIDriverWrite(unsigned char byte);

