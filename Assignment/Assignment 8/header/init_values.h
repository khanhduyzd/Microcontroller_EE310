#ifndef INIT_VALUES_H
#define INIT_VALUES_H

#include <xc.h>

#define _XTAL_FREQ 1000000

// ==========================
// USER SETTINGS
// ==========================

// Secret code digits
#define SECRET_DIGIT1  2
#define SECRET_DIGIT2  3

// Assignment says input values will not exceed 3
#define MAX_INPUT_DIGIT 3

// Timing
#define SENSOR_DEBOUNCE_MS      250
#define DIGIT_DONE_TIMEOUT_MS   1500
#define MOTOR_ON_TIME_MS        4000
#define WRONG_CODE_BUZZ_MS      2000

// ==========================
// SIGNAL POLARITY
// Change these if your hardware is opposite
// ==========================

// Photoresistor active when covered -> LOW
#define PR_ACTIVE_LEVEL 0

// Relay module polarity
// 0 = active LOW relay module
// 1 = active HIGH relay module
#define RELAY_ACTIVE_LEVEL 0

// Buzzer polarity
#define BUZZER_ACTIVE_LEVEL 1

// LED polarity
#define LED_ACTIVE_LEVEL 1

// 7-segment type
// 0 = common cathode
// 1 = common anode
#define SEVEN_SEG_COMMON_ANODE 0

#endif
