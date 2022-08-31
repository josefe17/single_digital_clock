/*
 * GPIO.c
 *
 * Created: 01/11/2017 14:28:54
 *  Author: josefe
 */ 

#include "GPIO.h"
#include <avr/io.h>

volatile unsigned char button_scanning_row_counter;
volatile unsigned int button_flags_buffer;

volatile unsigned char* const ROWS_DRIVE_PORTS[] = {&PORTB, &PORTD};
const unsigned char ROWS_DRIVE_PORT_BITS[] = {PORTB2, PORTD2};
volatile unsigned char* const COLUMNS_READ_PINS[] = {&PINB, &PINC, &PINC, &PIND};
const unsigned char COLUMNS_READ_PIN_BITS[] = {PINB4, PINC0, PINC1, PIND0};

void gpio_init(void)
{
	button_flags = 0;
	button_flags_buffer = 0;
	button_scanning_row_counter = 0;
	
	//Enable pull up
	MCUCR&=~(1<<PUD);		
	
	//Set directions
	DDRB|=(GPIO_DDRB_MASK & GPIO_PORTB_MASK);
	DDRC|=(GPIO_DDRC_MASK & GPIO_PORTC_MASK);
	DDRD|=(GPIO_DDRD_MASK & GPIO_PORTD_MASK);
		
	//Enable local pullup or set reset level
	PORTB|=(PORTB_RESET_VALUE & GPIO_PORTB_MASK);	
	PORTC|=(PORTC_RESET_VALUE & GPIO_PORTC_MASK);
	PORTD|=(PORTD_RESET_VALUE & GPIO_PORTD_MASK);
}

void update_clock_output(unsigned char value)
{
	if (value)
	{
		GPIO_CLOCK_2HZ_PORT |= (1 << GPIO_CLOCK_2HZ);
	}
	else
	{
		GPIO_CLOCK_2HZ_PORT &= ~(1 << GPIO_CLOCK_2HZ);
	}
}

void process_button_scanning_sequence(void)
{
	if (button_scanning_row_counter < NUMBER_OF_ROWS)
	{
		for (unsigned char i=0; i < NUMBER_OF_ROWS; ++i) //For all rows
		{
			if (button_scanning_row_counter == i)
			{
				*(ROWS_DRIVE_PORTS[i]) &= ~(1 << ROWS_DRIVE_PORT_BITS[i]); //Drive the current low
			}
			else
			{
				*(ROWS_DRIVE_PORTS[i]) |= (1 << ROWS_DRIVE_PORT_BITS[i]); //Set the rest to high
			}
		}
		for (unsigned char i=0; i < NUMBER_OF_COLUMNS; ++i) // For each column
		{
			// Check wheter each button is pressed and set the proper flag at the flag buffer according to its matrix position (see below)
			button_flags_buffer |= ((~( (*(COLUMNS_READ_PINS[i])) >> COLUMNS_READ_PIN_BITS[i]) & 1)  << ((button_scanning_row_counter * NUMBER_OF_COLUMNS) + i));
		}
		++button_scanning_row_counter;
	}
	else
	{
		for (unsigned char i=0; i < NUMBER_OF_ROWS; ++i) //For all rows
		{
			*(ROWS_DRIVE_PORTS[i]) |= (1 << ROWS_DRIVE_PORT_BITS[i]); //Set the rest to high
		}
		button_scanning_row_counter = 0;
		button_flags = button_flags_buffer;
		button_flags_buffer = 0;
	}		
}



