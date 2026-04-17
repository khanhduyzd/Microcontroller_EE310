#ifndef INIT_VALUES_H
#define INIT_VALUES_H

#include <xc.h>

#define _XTAL_FREQ 1000000UL

// Secret code
#define SECRET_DIGIT1  2
#define SECRET_DIGIT2  3

// values do not exceed 4
#define MAX_INPUT_DIGIT         4

// Timing
#define SENSOR_DEBOUNCE_MS      250
#define DIGIT_DONE_TIMEOUT_MS   1300
#define MOTOR_ON_TIME_MS        2000
#define WRONG_CODE_BUZZ_MS      3000

// Logic polarity
// Change only if your hardware behaves opposite
#define PR_ACTIVE_LEVEL         0   // covered/dark = LOW
#define RELAY_ACTIVE_LEVEL      1   // RD7 HIGH turns relay ON
#define BUZZER_ACTIVE_LEVEL     1   // RC2 HIGH turns buzzer ON
#define LED_ACTIVE_LEVEL        1   // RE1 HIGH turns LED ON

// 7-segment type
#define SEVEN_SEG_COMMON_ANODE  0   // 0 = common cathode

#endif
