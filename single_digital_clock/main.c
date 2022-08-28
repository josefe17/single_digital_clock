
#define F_CPU 20000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "data_casting.h"
#include "common_anode_display_driver.h"
#include "rtc.h"
#include "TWI_Master.h"
#include "timer0_tick.h"
#include "fsm.h"
#include "GPIO.h"

#define MS_10_TIMER_COUNT	194
#define MS_10_DELAY_CYCLES	5

#define FAST_SKIP_TIMER_DELAY_CYCLES 20 //system delay (50 ms) * 20 = 1 s

/*HW ADDRESSES*/
#define HT16K33_1_WRITE_ADDRESS	0b11100000
#define HT16K33_2_WRITE_ADDRESS	0b11100010
#define DS3231_WRITE_ADDRESS	0b11010000
#define BRIGHTNESS_CHANNEL		3

/*Data buffers*/
unsigned char str_buffer[8]="        ";
volatile time_data current_time;
unsigned char power_fail_flag;

/*Fast skip timer resources*/
unsigned char fast_skip_count;
unsigned char fast_skip_counter;
unsigned char fast_skip_enable;

/*Prototypes*/
/*ADC*/
void InitADC(void);
unsigned char ReadADC(unsigned char ADCchannel);
/*Fast skip timer*/
void start_fast_skip_timer(unsigned char timeout);
void stop_fast_skip_timer(void);
unsigned char check_fast_skip_timer(fsm_t* this);
void increment_fast_skip_timer(void);

/*Clock FSM input functions*/
unsigned char check_no_buttons(fsm_t* this);
unsigned char check_skip_long_press(fsm_t* this);
unsigned char check_any_buttons(fsm_t* this);
unsigned char check_idle(fsm_t* this);
unsigned char check_set_clock(fsm_t* this);
unsigned char check_set(fsm_t* this);
unsigned char check_clock(fsm_t* this);
unsigned char check_arrows(fsm_t* this);
unsigned char check_blinking_flag(fsm_t* this);
unsigned char check_no_blinking_flag(fsm_t* this);

/*Clock FSM output functions*/
void showtime(fsm_t* this);
void showtime_and_stop_timer(fsm_t* this);
void showYear (fsm_t* this);
void showYear_and_stop_timer(fsm_t* this);
void updatetime(fsm_t* this);
void blinktime(fsm_t* this);
void clear_blinktime_and_showtime(fsm_t* this);
void clear_oscillator_fault_and_showtime(fsm_t* this);
void blinkhour(fsm_t* this);
void modify_hour(fsm_t* this);
void modify_hour_once(fsm_t* this);
void blinkminute(fsm_t* this);
void modify_minute(fsm_t* this);
void modify_minute_once(fsm_t* this);
void blinkdayM (fsm_t* this);
void modify_dayM(fsm_t* this);
void modify_dayM_once(fsm_t* this);
void blinkMonth (fsm_t* this);
void modify_month(fsm_t* this);
void modify_month_once(fsm_t* this);
void blinkYear (fsm_t* this);
void modify_year(fsm_t* this);
void modify_year_once(fsm_t* this);

/*Clock FSM states*/
typedef enum
{
	NULL_STATE,
	IDLE,
	DEBOUNCE_IDLE,
	BLINKING,
	DEBOUNCE_BLINKING,	
	SETHOUR,
	SETHOUR_FAST,
	DEBOUNCE_SETHOUR,
	SETMIN,
	SETMIN_FAST,
	DEBOUNCE_SETMIN
	}clock_states;

int main (void)
{	
	volatile unsigned char adc_buffer=0;
		
	volatile fsm_t clock_fsm; //Clock FSM model
	
	fsm_trans_t clock_tt[] = {
	{ DEBOUNCE_IDLE,	check_no_buttons,		IDLE,				showtime	},	//If buttons are released got next state
	{ DEBOUNCE_IDLE,	check_any_buttons,		DEBOUNCE_IDLE,		showtime	},	// else wait for release in debounce state
	{ IDLE,				check_blinking_flag,	BLINKING, 			blinktime	},	// If clock fail, display starts blinking
	{ IDLE,				check_set,				DEBOUNCE_SETHOUR, 	showtime	},	// If clock and set are pressed, goes to clock adjustment		
	{ IDLE,				check_idle,				IDLE, 				showtime	},  // just updates display
	
	{ BLINKING,			check_set,				DEBOUNCE_SETHOUR, 	clear_oscillator_fault_and_showtime	},	//If while blinking, clock and set are pressed, turn off global blinking adnd goes to clock adjustment (debounce hour)
	{ BLINKING,			check_blinking_flag,	BLINKING, 			blinktime							},	//Else keep blinking
	{ BLINKING,			check_no_blinking_flag,	IDLE, 				clear_blinktime_and_showtime		},	//if flag is HW cleared, goes back to normal mode		
	
	{DEBOUNCE_SETHOUR,	check_no_buttons,		SETHOUR,			blinkhour	},				//If we are on debounce mode and buttons are released, goes to single adjustment and forces hour to blink
	{DEBOUNCE_SETHOUR,	check_skip_long_press,	SETHOUR_FAST,		showtime_and_stop_timer	},	//If we are on debounce mode and long press is being produced (timeout and arrows long press), goes to fast skip
	{DEBOUNCE_SETHOUR,	check_any_buttons,		DEBOUNCE_SETHOUR,	showtime	},				//If we are on debounce mode and buttons are still pressed and no timeout has been produced, keeps waiting		
	{SETHOUR_FAST,		check_arrows,			SETHOUR_FAST,		modify_hour	},				//If we are on fast skip and arrows are hold down, hour is increased or decreased rapidly (once or each fsm cycle)
	{SETHOUR_FAST,		check_no_buttons,		SETHOUR,			blinkhour	},				//If we are on fast skip and no keys are pressed, goes to single adjustment and forces hour to blink 
	{ SETHOUR,			check_no_buttons,		SETHOUR, 			blinkhour	},				//If we are on single adjustment and no buttons are pressed, hour keeps blinking
	//{ SETHOUR,			check_clock,			DEBOUNCE_IDLE, 		updatetime	},				//If we are on single adjustment and clock is pressed, time is stored (fake, is stored each time is incremented o decremented) and goes to normal view
	{ SETHOUR,			check_set,				DEBOUNCE_SETMIN, 	blinkminute	},				//If we are on single adjustment and set is pressed, the minutes start blinking and can be modified now
	{ SETHOUR,			check_arrows,			DEBOUNCE_SETHOUR, 	modify_hour_once},			//If we press any of both arrows, clock will increment or decrement once and will go to debounce state waiting for release or long press (also starts fast skip timer)
		
	{DEBOUNCE_SETMIN,	check_no_buttons,		SETMIN,				blinkminute	},				//Minute adjustment, identical as hour
	{DEBOUNCE_SETMIN,	check_skip_long_press,	SETMIN_FAST,		showtime_and_stop_timer	},
	{DEBOUNCE_SETMIN,	check_any_buttons,		DEBOUNCE_SETMIN,	showtime	},	
	{SETMIN_FAST,		check_arrows,			SETMIN_FAST,		modify_minute	},
	{SETMIN_FAST,		check_no_buttons,		SETMIN,				blinkminute	},		
	{ SETMIN,			check_no_buttons,		SETMIN, 			blinkminute	},
	//{ SETMIN,			check_clock,			DEBOUNCE_IDLE,		updatetime	},	
	{ SETMIN,			check_set,				DEBOUNCE_IDLE,		updatetime	},		
	{ SETMIN,			check_arrows,			DEBOUNCE_SETMIN,	modify_minute_once	},			
			
	{ -1,			NULL_STATE,				-1,			NULL_STATE		}
	};
	
	
	fsm_init(&clock_fsm, IDLE, clock_tt, 0); //starts FSM
	
	gpio_init();	//Inits GPIO												
	TWI_Master_Initialise(); //Inits I2C
	InitADC();	
	display_init(0);
	sei();	
	display_update(0, DISPLAY_TYPE_DVBX5210_PS2, str_buffer, 0, 0);
	adc_buffer=(ReadADC(3));
	set_brightness_display(0, adc_buffer);	
	timer0_tick_init(T0_PRESCALER_1024, MS_10_TIMER_COUNT, MS_10_DELAY_CYCLES);	
	
	while(1)
	{	
		current_time=retrieve_timestamp_from_RTC(DS3231_WRITE_ADDRESS, DATA_DECIMAL);
		power_fail_flag=check_oscillator_fault(DS3231_WRITE_ADDRESS);
		increment_fast_skip_timer();
		update_button_flags();
		adc_buffer=(ReadADC(3));
		set_brightness_display(0,adc_buffer);
		fsm_fire(&clock_fsm);			
		delay_until_tick();	
		
//
//#ifdef temperature
		//current_time=retrieve_timestamp_from_RTC(DS3231_WRITE_ADDRESS, DATA_BCD);
		//str_buffer[0]=bcd2char(current_time.hour>>4);
		//str_buffer[1]=bcd2char(current_time.hour);
		//str_buffer[2]=bcd2char(current_time.min>>4);
		//str_buffer[3]=bcd2char(current_time.min);
		//
		//temperature_C=get_temperature(DS3231_WRITE_ADDRESS);
		//if (temperature_C<0)
		//{
			//str_buffer[4]='-';
			//temperature_C*=-1;
		//}
		//else
		//{
			//str_buffer[4]=' ';
		//}
		//aux=(unsigned char) (temperature_C>>8); //Casted to char
		//str_buffer[6]=bcd2char(dec2bcd(aux));
		//str_buffer[5]=bcd2char((dec2bcd(aux))>>4);		
		//str_buffer[7]='C';		
		//display_update(HT16K33_WRITE_ADDRESS, str_buffer, 0, SECOND_DEGREES_DOT_MASK | (FIRST_COLON_MASK & current_time.sec));
		//set_brightness_display(HT16K33_WRITE_ADDRESS,(ReadADC(3)>>4)&0x0F);	
		//delay_until_tick();	
//#endif // temperature
//#ifdef calendar
		//current_time=retrieve_timestamp_from_RTC(DS3231_WRITE_ADDRESS, DATA_BCD);
		//str_buffer[0]=bcd2char(current_time.dayM>>4);
		//str_buffer[1]=bcd2char(current_time.dayM);
		//str_buffer[2]=bcd2char(current_time.month>>4);
		//str_buffer[3]=bcd2char(current_time.month);
		//str_buffer[4]=bcd2char((unsigned char) (current_time.year>>12));
		//str_buffer[5]=bcd2char((unsigned char) (current_time.year>>8));
		//str_buffer[6]=bcd2char((unsigned char) (current_time.year>>4));
		//str_buffer[7]=bcd2char((unsigned char) current_time.year);
		//display_update(HT16K33_WRITE_ADDRESS, str_buffer, (1<<1)|(1<<3), 0);
		//set_brightness_display(HT16K33_WRITE_ADDRESS,(ReadADC(3)>>4)&0x0F);
		//delay_until_tick();		
//#endif // calendar
		
	}	
}

/*FSM input functions*/
unsigned char check_no_buttons(fsm_t* this)
{
	return (button_flags==0);
}

unsigned char check_skip_long_press(fsm_t* this)
{
	return (check_arrows(this) && check_fast_skip_timer(this));
}

unsigned char check_any_buttons(fsm_t* this)
{
	return button_flags;
}

unsigned char check_idle(fsm_t* this)
{
	return ((button_flags & (~SET_BUTTON_MASK))==0); //If no buttons without set are pressed
}

unsigned char check_set_clock(fsm_t* this)
{
	return ((button_flags & SET_BUTTON_MASK) && (button_flags & CLOCK_BUTTON_MASK)); //If clock and set are pressed
}

unsigned char check_set(fsm_t* this)
{
	return (button_flags & SET_BUTTON_MASK);
}

unsigned char check_clock(fsm_t* this)
{
	return (button_flags & CLOCK_BUTTON_MASK);
}

unsigned char check_arrows(fsm_t* this)
{
	return (button_flags && (UP_BUTTON_MASK | DOWN_BUTTON_MASK));
}

unsigned char check_blinking_flag(fsm_t* this)
{
	return power_fail_flag;
}

unsigned char check_no_blinking_flag(fsm_t* this)
{
	return (!power_fail_flag);
}

/*FSM Output functions*/
void start_fast_skip_timer(unsigned char timeout)
{
	fast_skip_count=timeout;
	fast_skip_counter=0;
	fast_skip_enable=1;	
}

void stop_fast_skip_timer(void)
{
	fast_skip_enable=0;
}

unsigned char check_fast_skip_timer(fsm_t* this)
{
	return (fast_skip_enable && (fast_skip_counter>=fast_skip_count));
}

void increment_fast_skip_timer(void)
{
	if (fast_skip_enable)
	{
		if (fast_skip_counter<255)
		{
			++fast_skip_counter;
		}
	}
	else
	{
		fast_skip_counter=0;
	}
}

void showtime(fsm_t* this)
{	
	str_buffer[0]=bcd2char(dec2bcd(current_time.hour)>>4);
	if (str_buffer[0]=='0')
	{
		str_buffer[0]=' ';
	}
	str_buffer[1]=bcd2char(dec2bcd(current_time.hour));
	str_buffer[2]=bcd2char(dec2bcd(current_time.min)>>4);
	str_buffer[3]=bcd2char(dec2bcd(current_time.min));
	display_update(0, DISPLAY_TYPE_DVBX5210_PS2, str_buffer, 0, (1 & current_time.sec)<<3);
}

void showtime_and_stop_timer(fsm_t* this)
{
	stop_fast_skip_timer();
	showtime(this);
}	

void updatetime(fsm_t* this)
{
	current_time=getDayOfWeek(current_time); //Updates dayW	
	update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
	showtime(this);
}

void blinktime(fsm_t* this)
{
	if (1 & current_time.sec)
	{
	showtime(this);	
	}
	else 
	{
		str_buffer[0]=' ';
		str_buffer[1]=' ';
		str_buffer[2]=' ';
		str_buffer[3]=' ';
		str_buffer[4]=' ';
		str_buffer[5]=' ';
		str_buffer[6]=' ';
		str_buffer[7]=' ';
		display_update(0, DISPLAY_TYPE_DVBX5210_PS2, str_buffer, 0, 0);
	}	
}

void clear_blinktime_and_showtime( fsm_t* this )
{
	showtime(this);

}

void clear_oscillator_fault_and_showtime(fsm_t* this)
{
	clear_oscillator_fault(DS3231_WRITE_ADDRESS);
	showtime(this);
}

void blinkhour(fsm_t* this)
{	
	stop_fast_skip_timer();
	if (1 & current_time.sec)
	{
		str_buffer[0]=bcd2char(dec2bcd(current_time.hour)>>4);
		if (str_buffer[0]=='0')
		{
			str_buffer[0]=' ';
		}
		str_buffer[1]=bcd2char(dec2bcd(current_time.hour));
	}
	else
	{
		str_buffer[0]=' ';
		str_buffer[1]=' ';
	}
	str_buffer[2]=bcd2char(dec2bcd(current_time.min)>>4);
	str_buffer[3]=bcd2char(dec2bcd(current_time.min));
	display_update(0, DISPLAY_TYPE_DVBX5210_PS2, str_buffer, 0, (1 & current_time.sec)<<3);
	
}


void modify_hour_once(fsm_t* this)
{
	start_fast_skip_timer(FAST_SKIP_TIMER_DELAY_CYCLES);
	modify_hour(this);
}

void modify_hour(fsm_t* this)
{
	if ((button_flags & UP_BUTTON_MASK) && (button_flags & DOWN_BUTTON_MASK))
	{
		showtime(this);
		return;
	}
	
	if (button_flags & UP_BUTTON_MASK)
	{
		++current_time.hour;
		if (current_time.hour>=HOURS_OF_DAY)
		{
			current_time.hour=0;
		}
		current_time.sec=0;
		update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
		showtime(this);
		return;		
	}
	if (button_flags & DOWN_BUTTON_MASK)
	{
		if (current_time.hour==0)
		{
			current_time.hour=(HOURS_OF_DAY-1);
		}
		else
		{
			--current_time.hour;
		}
		current_time.sec=0;
		update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
		showtime(this);
		return;
	}
	update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
	showtime(this);	
}


void blinkminute(fsm_t* this)
{
	stop_fast_skip_timer();
	if (1 & current_time.sec)
	{
		str_buffer[2]=bcd2char(dec2bcd(current_time.min)>>4);
		str_buffer[3]=bcd2char(dec2bcd(current_time.min));
	}
	else
	{
		str_buffer[2]=' ';
		str_buffer[3]=' ';
	}
	str_buffer[0]=bcd2char(dec2bcd(current_time.hour)>>4);
	if (str_buffer[0]=='0')
	{
		str_buffer[0]=' ';
	}
	str_buffer[1]=bcd2char(dec2bcd(current_time.hour));
	display_update(0, DISPLAY_TYPE_DVBX5210_PS2, str_buffer, 0, (1 & current_time.sec)<<3);
}

void modify_minute_once(fsm_t* this)
{
	start_fast_skip_timer(FAST_SKIP_TIMER_DELAY_CYCLES);
	modify_minute(this);
}

void modify_minute(fsm_t* this)
{
	if ((button_flags & UP_BUTTON_MASK) && (button_flags & DOWN_BUTTON_MASK))
	{
		showtime(this);
		return;
	}
	
	if (button_flags & UP_BUTTON_MASK)
	{
		++current_time.min;
		if (current_time.min>=MINUTES_OF_HOUR)
		{
			current_time.min=0;
		}
		current_time.sec=0;
		update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
		showtime(this);
		return;
	}
	if (button_flags & DOWN_BUTTON_MASK)
	{
		if (current_time.min==0)
		{
			current_time.min=MINUTES_OF_HOUR-1;
		}
		else
		{
			--current_time.min;
		}
		current_time.sec=0;
		update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
		showtime(this);
		return;
	}
	update_timestamp_to_RTC(DS3231_WRITE_ADDRESS, current_time, DATA_DECIMAL);
	showtime(this);
}


void InitADC(void)
{
	// Select Vref=AVcc & Left justified
	ADMUX |= (1<<REFS0) | (1<<ADLAR);
	//set prescaler to 128 and enable ADC
	ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
}

unsigned char ReadADC(unsigned char ADCchannel)
{
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADCH;
}



	
