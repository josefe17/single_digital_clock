/*
 * nativeSPIDriver.c
 *
 * Created: 24/08/2022 21:27:14
 *  Author: josefe
 */ 

#include "nativeSPIDriver.h"
#include <avr/io.h>

void nativeSPIDriverInit()
{
	/* Set MOSI and SCK output, all others input */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void nativeSPIDriverWrite(unsigned char data)
{
	/* Start transmission */
	PORTB|= (1<<PORTB2);
	SPDR = data;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));	
	//PORTB&= ~(1<<PORTB2);
	(void) SPDR;
	//PORTB&= ~(1<<PORTB2);
}