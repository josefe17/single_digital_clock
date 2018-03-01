/*
 * GPIO.h
 *
 * Created: 01/11/2017 14:41:10
 *  Author: josefe
 */ 


#ifndef GPIO_H_
#define GPIO_H_

#define GPIO_PORTB_MASK		0b00000000		//Pins used as gpio
#define GPIO_PORTC_MASK		0b00000111
#define GPIO_PORTD_MASK		0b00000000

#define	GPIO_DDRB_MASK		0b00000000	 
#define GPIO_DDRC_MASK		0b00000000		//All inputs
#define GPIO_DDRD_MASK		0b00000000

#define PORTB_RESET_VALUE	0b00000000		//1 enables pullups or sets 1 logic in outputs
#define PORTC_RESET_VALUE	0b00000111
#define PORTD_RESET_VALUE	0b00000000

#define PC_GPIO0 (PINC & (1<<PINC0))
#define PC_GPIO1 (PINC & (1<<PINC1))

#define PD_GPIO0 (PIND & (1<<PIND0))
#define PD_GPIO1 (PIND & (1<<PIND1))			
#define PD_GPIO4 (PIND & (1<<PIND4))
#define PD_GPIO5 (PIND & (1<<PIND5))
#define PD_GPIO6 (PIND & (1<<PIND6))
#define PD_GPIO7 (PIND & (1<<PIND7))

#define SET_BUTTON_MASK		0b0000000010000000 //PC2
#define UP_BUTTON_MASK		0b0000000001000000 //PC1
#define DOWN_BUTTON_MASK	0b0000000000100000 //PC0

#define CLOCK_BUTTON_MASK	0	//Compatibility mode

volatile unsigned int button_flags; //A&B

void gpio_init(void);
void update_button_flags(void);

#endif /* GPIO_H_ */