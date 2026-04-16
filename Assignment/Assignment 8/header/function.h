#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <xc.h>

// Global flags
extern volatile unsigned char emergency_flag;
extern volatile unsigned char system_halted;

// Initialization
void system_init(void);

// Output control
void led_on(void);
void led_off(void);

void relay_on(void);
void relay_off(void);

void buzzer_on(void);
void buzzer_off(void);

// Display
void sevenseg_show(unsigned char digit);
void sevenseg_blank(void);

// Delay helpers
void delay_variable_us(unsigned int us);
void delay_ms_block(unsigned int ms);

// Melody
void play_tone(unsigned int half_period_us, unsigned int cycles);
void play_emergency_melody(void);

// Sensor helpers
unsigned char pr1_active(void);
unsigned char pr2_active(void);

// Digit capture
unsigned char capture_digit_from_pr1(void);
unsigned char capture_digit_from_pr2(void);

// Actions
void wrong_code_alarm(void);
void correct_code_action(void);

// ISR is implemented in functions.c

#endif
