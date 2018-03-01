#include "common_anode_display_driver.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define zero_logic

volatile unsigned char data_buffer[DISPLAY_BUFFER_SIZE]={DIGIT_DISABLED, DIGIT_DISABLED, DIGIT_DISABLED, DIGIT_DISABLED};
volatile unsigned char enable_buffer[DISPLAY_BUFFER_SIZE]={DIGIT_0_MASK, DIGIT_1_MASK, DIGIT_2_MASK, DIGIT_3_MASK};
volatile unsigned char buffer_index;

unsigned char ascii_2_7segment(unsigned char character);

ISR (TIMER1_OVF_vect)
{
#ifdef zero_logic
	ENABLE_PORT |= ENABLE_PORT_MASK; 				//Disable display (0 logic)
#else
	ENABLE_PORT &= ~ENABLE_PORT_MASK; 				//Disable display (1 logic)
#endif
	DATA_PORT = data_buffer[buffer_index];			//Update data (negative values already inverted)
#ifdef zero_logic
	ENABLE_PORT &= ~enable_buffer[buffer_index];	//Re-enable display on the following digit (enable buffer contents are already masked, 0 logic)
#else
	ENABLE_PORT |= enable_buffer[buffer_index];		//Re-enable display on the following digit (enable buffer contents are already masked, 1 logic)
#endif
	++buffer_index;
	if (buffer_index>=DISPLAY_BUFFER_SIZE)		//Refreshing rate is 13ms = 76,3 Hz
	{
		buffer_index=0;						
	}
}

void display_init(unsigned char I2C_display_address)
{
	buffer_index=0; 			//Clears data	
	ENABLE_DDR = (ENABLE_DDR & (~ENABLE_PORT_MASK)) | (ENABLE_DDR_CONTENT & ENABLE_PORT_MASK); //Set DDR's
	DATA_DDR = DATA_DDR_CONTENT;
	//ENABLE_PORT &= ~ENABLE_PORT_MASK; 	//Disables display, 1 logic
	ENABLE_PORT |= ENABLE_PORT_MASK; 	//Disables display, 0 logic
	DATA_PORT |= DIGIT_DISABLED;
	PORTB&=~(1<<PORTB1); //Timer PWM output on pin 15 (PB1, OC1A)
	DDRB|=(1<<PORTB1);
	//Timer init
	TCNT1H=0;
	TCNT1L=0;
	OCR1AH=0;
	OCR1AL=0;	
#ifdef zero_logic
	TCCR1A=0b11000001;			//Fast 8 bit PWM on OC1A (0 logic)
#else
	TCCR1A=0b10000001;			//Fast 8 bit PWM on OC1A (1 logic)
#endif	
	TCCR1B=0b00001100;			//Fast PWM and fclk/256 prescaler (PWM modulation is embedded inside multiplexing duty cylce)
	TIFR1|=(1<<TOV1);			//Interrupts enable on timer1 overflow vector (freq is set t fclk/(256+256) = 305Hz = 3,2s)
	TIMSK1|=(1<<TOIE1);
	
}

void display_update(unsigned char I2C_display_address, unsigned char display_type, unsigned char* data_string, unsigned char decimal_dots_mask, unsigned char special_dots_mask)
{
	unsigned char i;			//Copies and inverts data
	for (i=0; i<DISPLAY_BUFFER_SIZE; ++i)
	{
		data_buffer[i] = ~(ascii_2_7segment(data_string[i]));
	}
	data_buffer[1]&=~ special_dots_mask;
}

void dots_update(unsigned char I2C_display_address, unsigned char display_type, unsigned char decimal_dots_mask, unsigned char special_dots_mask)
{
	data_buffer[1]|= special_dots_mask;
}

void clear_display(unsigned char I2C_display_address)
{
	data_buffer[0]=DIGIT_DISABLED;
	data_buffer[1]=DIGIT_DISABLED;
	data_buffer[2]=DIGIT_DISABLED;
	data_buffer[3]=DIGIT_DISABLED;
}

void set_brightness_display(unsigned char I2C_display_address, unsigned char brightness)
{
	OCR1AL=brightness;			//Copies data
}

void turn_on_and_blink_display(unsigned char I2C_display_address, unsigned char display_blinking_mode);
void turn_off_display(unsigned char I2C_display_address);


//Casts between ASCII char value to 7 segments display mask
unsigned char ascii_2_7segment(unsigned char character)
{
	switch (character)
	{
		case '0': return 0b00111111;
		case '1': return 0b00000110;
		case '2': return 0b01011011;
		case '3': return 0b01001111;
		case '4': return 0b01100110;
		case '5': return 0b01101101;
		case '6': return 0b01111101;
		case '7': return 0b00000111;
		case '8': return 0b01111111;
		case '9': return 0b01101111;

		case 'a':
		case 'A': return 0b01110111;
		case 'b':
		case 'B': return 0b01111100;
		case 'c': return 0b01011000;
		case 'C': return 0b00111001;
		case 'd':
		case 'D': return 0b01011110;
		case 'e': return 0b01111011;
		case 'E': return 0b01111001;
		case 'f':
		case 'F': return 0b01110001;
		case 'g': return 0b01101111;
		case 'G': return 0b00111101;
		case 'h': return 0b01110100;
		case 'H': return 0b01110110;
		case 'i': return 0b00010000;
		case 'I': return 0b00110000;
		case 'j': return 0b00011110;
		case 'J': return 0b00011111;
		case 'k':
		case 'K': return 0b01110000;
		case 'l': return 0b00110000;
		case 'L': return 0b00111000;
		case 'm': return 0b01010100;
		case 'M': return 0b00110111;
		case 'n': return 0b01010100;
		case 'N': return 0b00110111;
		case 164: //ñ
		case 165: return 0b01010101;
		case 'o': return 0b01011100;
		case 'O': return 0b00111111;
		case 'p':
		case 'P': return 0b01110011;
		case 'q': return 0b11011100;
		case 'Q': return 0b10111111;
		case 'r': return 0b01010000;
		case 'R': return 0b00110001;
		case 's':
		case 'S': return 0b01101101;
		case 't': return 0b01111000;
		case 'T': return 0b00110001;
		case 'u': return 0b00011100;
		case 'U': return 0b00111110;
		case 'v': return 0b00011100;
		case 'V': return 0b00111110;
		case 'w': return 0b10011100;
		case 'W': return 0b10111110;
		case 'x':
		case 'X': return 0b01110110;
		case 'y': return 0b01101110;
		case 'Y': return 0b01101110;
		case 'z':
		case 'Z': return 0b01101101;

		case ' ': return 0;
		case '-': return 0b01000000;
		case '!': return 0b10000010;
		case ':':
		case '.':
		case ',': return 0b10000000;

		case '_':
		default: return 0b00001000; //_

	}
}
	