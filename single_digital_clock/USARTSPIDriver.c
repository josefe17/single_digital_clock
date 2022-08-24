/*
 * USARTSPIDriver.c
 *
 * Created: 24/08/2022 21:39:14
 *  Author: josefe
 */ 

#include "USARTSPIDriver.h"
#include <avr/io.h>


void USARTSPIDriverInitTXOnly()
{
	UBRR0 = 0;
	/* Setting the XCKn port pin as output, enables master mode. */
	XCK0_DDR |= (1<<XCK0);
	/* Set MSPI mode of operation and SPI data mode 0. */
	UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(0<<UCPHA0)|(0<<UCPOL0);
	/* Enable transmitter. */
	UCSR0B = (1<<TXEN0);
	/* Set baud rate. */
	/* IMPORTANT: The Baud Rate must be set after the transmitter is
	enabled
	*/
	UBRR0 = USARTSPIBAUD;
}

void USARTSPIDriverWrite(unsigned char byte)
{
	/* Wait for empty transmit buffer */
	while (!(UCSR0A & (1<<UDRE0)));
	/* Put data into buffer, sends the data */
	UDR0 = byte;
}