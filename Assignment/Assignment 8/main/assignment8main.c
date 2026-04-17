/***************************************************************
/ FileName: Safebox System Design
/ Author: Duy Pham
/ Course: EE310
/ Date: 04/16/26
/
/ MPLAB Version: MPLAB X IDE v6.30
/ Operating System: Windows 11
/ Program Version: V1 - Final Version
/
/ Purpose:
/   Implement a safebox system using a PIC18F47K42 microcontroller that allows a user to enter a two-digit secret code using touchless photoresistor inputs. 
/ The system controls a relay (motor) and buzzer based on correct or incorrect code entry and includes an emergency interrupt feature.
/
/ Inputs:
/   RA0 - PR1 (Photoresistor 1, first digit input)
/   RA1 - PR2 (Photoresistor 2, second digit input)
/   RB1 - Emergency Switch (Interrupt input)
/
/ Outputs:
/   RD0–RD6 - 7-segment display (digit output)
/   RD7     - Relay control (motor activation)
/   RC2     - Buzzer (alarm and emergency melody)
/   RE1     - LED (indicates digit confirmation)
/
/ Description:
/   The system reads two photoresistor inputs (PR1 and PR2) to allow the user to enter a two-digit secret code. Each time the user blocks light to a photoresistor, 
/ the count increments. After a short period of inactivity, the digit is confirmed, displayed on the 7-segment display.
/   Once both digits are entered, the system compares them to a predefined secret code:
/     - If correct, the relay is activated and the motor runs.
/     - If incorrect, the buzzer produces a long beep.
/   An emergency switch connected to RB1 triggers an interrupt at any time during operation. When activated, the system immediately
/
/   stops all activity and plays a distinct melody on the buzzer.
/ Instructions:
/   1. Cover PR1 repeatedly to enter the first digit.
/   2. Wait briefly for the digit to be confirmed (LED turns ON).
/   3. Cover PR2 repeatedly to enter the second digit.
/   4. Wait for confirmation of the second digit.
/   5. The system automatically checks the entered code.
/   6. Press the emergency switch (RB1) at any time to trigger
/      the interrupt and play the emergency melody.
***************************************************************/

#include "config_words.h"
#include "init_values.h"
#include "functions.h"

volatile unsigned char emergency_flag = 0;
volatile unsigned char ignore_emergency = 0;

// 7-segment patterns on RD0-RD6
// RD0=a, RD1=b, RD2=c, RD3=d, RD4=e, RD5=f, RD6=g
static const unsigned char seg_table[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};
// ----------------------------------
// Basic output control
// ----------------------------------
void led_on(void)
{
    LATEbits.LATE1 = LED_ACTIVE_LEVEL;
}

void led_off(void)
{
    LATEbits.LATE1 = !LED_ACTIVE_LEVEL;
}

void relay_on(void)
{
    LATDbits.LATD7 = RELAY_ACTIVE_LEVEL;
}

void relay_off(void)
{
    LATDbits.LATD7 = !RELAY_ACTIVE_LEVEL;
}

void buzzer_on(void)
{
    LATCbits.LATC2 = BUZZER_ACTIVE_LEVEL;
}

void buzzer_off(void)
{
    LATCbits.LATC2 = !BUZZER_ACTIVE_LEVEL;
}

// ----------------------------------
// Display
// ----------------------------------
void sevenseg_show(unsigned char digit)
{
    unsigned char pattern;

    if (digit > 9)
    {
        sevenseg_blank();
        return;
    }

    pattern = seg_table[digit];

#if SEVEN_SEG_COMMON_ANODE
    pattern = (~pattern) & 0x7F;
#endif

    // Keep RD7 unchanged because it drives the relay
    LATD = (LATD & 0x80) | pattern;
}

void sevenseg_blank(void)
{
#if SEVEN_SEG_COMMON_ANODE
    LATD = (LATD & 0x80) | 0x7F;
#else
    LATD = (LATD & 0x80) | 0x00;
#endif
}

// ----------------------------------
// Delay helpers
// ----------------------------------
void delay_variable_us(unsigned int us)
{
    unsigned int i;
    for (i = 0; i < us; i++)
    {
        __delay_us(1);
    }
}

void delay_ms_block(unsigned int ms)
{
    unsigned int i;
    for (i = 0; i < ms; i++)
    {
        __delay_ms(1);
    }
}

// ----------------------------------
// Emergency melody
// For an active buzzer: pulse pattern
// Total about 5 * (300+200) ms = 2.5 s
// ----------------------------------
void play_emergency_melody(void)
{
    unsigned char i;

    for (i = 0; i < 11; i++)
    {
        buzzer_on();
        __delay_ms(300);

        buzzer_off();
        __delay_ms(200);
    }

    buzzer_off();
}

// ----------------------------------
// Sensor helpers
// ----------------------------------
unsigned char pr1_active(void)
{
    return (PORTAbits.RA0 == PR_ACTIVE_LEVEL);
}

unsigned char pr2_active(void)
{
    return (PORTAbits.RA1 == PR_ACTIVE_LEVEL);
}

// ----------------------------------
// Digit capture using inactivity timeout
// LED turns on once the digit is confirmed
// ----------------------------------
static unsigned char capture_digit_generic(unsigned char (*sensor_func)(void))
{
    unsigned char count = 0;
    unsigned int idle_time = 0;

    led_off();

    while (1)
    {
        if (emergency_flag)
        {
            return 0;
        }

        if (sensor_func())
        {
            count++;

            if (count > MAX_INPUT_DIGIT)
            {
                count = MAX_INPUT_DIGIT;
            }

            sevenseg_show(count);

            delay_ms_block(SENSOR_DEBOUNCE_MS);

            while (sensor_func())
            {
                if (emergency_flag)
                {
                    return 0;
                }
            }

            idle_time = 0;
        }
        else
        {
            __delay_ms(10);
            idle_time += 10;

            if ((count > 0) && (idle_time >= DIGIT_DONE_TIMEOUT_MS))
            {
                led_on();
                return count;
            }
        }
    }
}

unsigned char capture_digit_from_pr1(void)
{
    return capture_digit_generic(pr1_active);
}

unsigned char capture_digit_from_pr2(void)
{
    return capture_digit_generic(pr2_active);
}

// ----------------------------------
// Actions
// ----------------------------------
void wrong_code_alarm()
{
        buzzer_on();
        __delay_ms(2000);
        buzzer_off();
     
}

void correct_code_action(void)
{
    unsigned int i;

    ignore_emergency = 1;

    buzzer_off();
    relay_on();

    // Motor ON period
    for (i = 0; i < MOTOR_ON_TIME_MS; i++)
    {
        __delay_ms(1);
    }

    // Still ignore during turn OFF (this is the missing fix)
    relay_off();

    // protect against OFF noise spike
    for (i = 0; i < 200; i++)
    {
        __delay_ms(1);
    }

    ignore_emergency = 0;

    buzzer_off();
}
// ----------------------------------
// Initialization
// ----------------------------------
void system_init(void)
{
    // Clock = 1 MHz HFINTOSC
    OSCCON1 = 0x60;
    OSCFRQ  = 0x00;

    // All digital
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;
    ANSELE = 0x00;

    // Inputs
    TRISAbits.TRISA0 = 1;   // PR1
    TRISAbits.TRISA1 = 1;   // PR2
    TRISBbits.TRISB1 = 1;   // Emergency switch

    // Outputs
    TRISCbits.TRISC2 = 0;   // Buzzer RC2
    TRISEbits.TRISE1 = 0;   // LED RE1
    TRISD = 0x00;           // RD0-RD7 outputs

    // Safe startup states
    LATDbits.LATD7 = !RELAY_ACTIVE_LEVEL;   // relay OFF immediately
    relay_off();
    buzzer_off();
    sevenseg_blank();
    led_off();

    // Emergency switch pull-up
    WPUBbits.WPUB1 = 1;

    // IOC on RB1 falling edge
    IOCBN = 0x00;
    IOCBP = 0x00;
    IOCBF = 0x00;

    IOCBNbits.IOCBN1 = 1;
    IOCBPbits.IOCBP1 = 0;

    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;
    INTCON0bits.GIE = 1;
}

// ----------------------------------
// Emergency ISR
// Melody must be in ISR
// ----------------------------------
void __interrupt(irq(IOC), base(8)) emergency_ISR(void)
{
    if (PIR0bits.IOCIF && IOCBFbits.IOCBF1)
    {
        if (ignore_emergency)
        {
            IOCBFbits.IOCBF1 = 0;
            PIR0bits.IOCIF = 0;
            return;
        }

        __delay_ms(20);

        if (PORTBbits.RB1 == 0)
        {
            emergency_flag = 1;

            relay_off();
            sevenseg_blank();
            led_off();

            play_emergency_melody();
        }

        IOCBFbits.IOCBF1 = 0;
        PIR0bits.IOCIF = 0;
    }
}

// ----------------------------------
// Main
// ----------------------------------
void main(void)
{
    unsigned char digit1;
    unsigned char digit2;

    system_init();

    while (1)
    {
        if (emergency_flag)
        {
            emergency_flag = 0;

            relay_off();
            buzzer_off();
            sevenseg_blank();
            led_off();

            while (PORTBbits.RB1 == 0)
            {
                ;
            }

            __delay_ms(50);
            continue;
        }

        // First digit from PR1
        digit1 = capture_digit_from_pr1();
        if (emergency_flag)
        {
            continue;
        }

        sevenseg_show(digit1);
        __delay_ms(200);

        // Turn LED off before second digit entry
        led_off();

        // Second digit from PR2
        digit2 = capture_digit_from_pr2();
        if (emergency_flag)
        {
            continue;
        }

        sevenseg_show(digit2);
        __delay_ms(200);

        if ((digit1 > 0) && (digit2 > 0) &&
            (digit1 == SECRET_DIGIT1) &&
            (digit2 == SECRET_DIGIT2))
        {
            correct_code_action();
        }
        else
        {
            wrong_code_alarm();
        }

        sevenseg_blank();
        led_off();
        __delay_ms(200);
    }
}
