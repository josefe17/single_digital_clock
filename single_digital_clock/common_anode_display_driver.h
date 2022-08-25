
#include <avr/io.h>

#define DISPLAY_BUFFER_SIZE 4
#define DIGIT_DISABLED 		255
//Defining digit's enables locations
#define DIGIT_0_MASK		0b00001000
#define DIGIT_1_MASK		0b00100000
#define DIGIT_2_MASK		0b01000000
#define DIGIT_3_MASK		0b10000000
#define ENABLE_PORT			PORTD
#define ENABLE_PORT_MASK	(DIGIT_0_MASK | DIGIT_1_MASK | DIGIT_2_MASK | DIGIT_3_MASK)
#define ENABLE_DDR			DDRD
#define ENABLE_DDR_CONTENT 	0b11101000 //As output

#define DATA_PORT			PORTD
#define DATA_DDR			DDRD
#define DATA_DDR_CONTENT 	0b11111111 //As output

/*Prototypes*/
void display_update(unsigned char I2C_display_address, unsigned char display_type, unsigned char* data_string, unsigned char decimal_dots_mask, unsigned char special_dots_mask);
void dots_update(unsigned char I2C_display_address, unsigned char display_type, unsigned char decimal_dots_mask, unsigned char special_dots_mask);
void clear_display(unsigned char I2C_display_address);
void display_init(unsigned char I2C_display_address);
void turn_on_and_blink_display(unsigned char I2C_display_address, unsigned char display_blinking_mode);
void turn_off_display(unsigned char I2C_display_address);
void set_brightness_display(unsigned char I2C_display_address, unsigned char brightness);