/*
 * GPIO.h
 *
 * Created: 01/11/2017 14:41:10
 *  Author: josefe
 */ 


#ifndef GPIO_H_
#define GPIO_H_

#define GPIO_PORTB_MASK		0b00010101		//Pins used as gpio
#define GPIO_PORTC_MASK		0b00000011
#define GPIO_PORTD_MASK		0b00000101

#define	GPIO_DDRB_MASK		0b00000101
#define GPIO_DDRC_MASK		0b00000000		//Rows and B0 as output
#define GPIO_DDRD_MASK		0b00000100

#define PORTB_RESET_VALUE	0b00010100		//1 enables pullups or sets 1 logic in outputs
#define PORTC_RESET_VALUE	0b00000011
#define PORTD_RESET_VALUE	0b00000101

#define GPIO_CLOCK_2HZ_PORT PORTB
#define GPIO_CLOCK_2HZ PORTB0

#define ADC_LDR_PIN ADC3D
#define ADC_THERMISTOR_PIN ADC2D

#define NUMBER_OF_ROWS 2
#define NUMBER_OF_COLUMNS 4

#define SET_BUTTON_MASK		0b10000000 // D, menu
#define UP_BUTTON_MASK		0b00000100 // C, vol+
#define DOWN_BUTTON_MASK	0b00100000 // B, vol-
#define CLOCK_BUTTON_MASK	0	//Compatibility mode

volatile unsigned char button_flags; //A&B
volatile unsigned char pind_debug;

void gpio_init(void);
void update_clock_output(unsigned char value);
void process_button_scanning_sequence(void);

#endif /* GPIO_H_ */