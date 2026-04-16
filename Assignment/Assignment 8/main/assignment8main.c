#include "config_words.h"
#include "init_values.h"
#include "functions.h"

volatile unsigned char emergency_flag = 0;
volatile unsigned char system_halted = 0;

// 7-segment patterns for digits 0-9
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
    LATCbits.LATC1 = RELAY_ACTIVE_LEVEL;
}

void relay_off(void)
{
    LATCbits.LATC1 = !RELAY_ACTIVE_LEVEL;
}

void buzzer_on(void)
{
    LATCbits.LATC2 = BUZZER_ACTIVE_LEVEL;
}

void buzzer_off(void)
{
    LATCbits.LATC2 = !BUZZER_ACTIVE_LEVEL;
}

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

    LATD = pattern;
}

void sevenseg_blank(void)
{
#if SEVEN_SEG_COMMON_ANODE
    LATD = 0x7F;
#else
    LATD = 0x00;
#endif
}

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

void play_tone(unsigned int half_period_us, unsigned int cycles)
{
    unsigned int i;

    for (i = 0; i < cycles; i++)
    {
        LATCbits.LATC2 = BUZZER_ACTIVE_LEVEL;
        delay_variable_us(half_period_us);

        LATCbits.LATC2 = !BUZZER_ACTIVE_LEVEL;
        delay_variable_us(half_period_us);
    }
}

void play_emergency_melody(void)
{
    play_tone(900, 120);
    __delay_ms(80);

    play_tone(600, 160);
    __delay_ms(80);

    play_tone(350, 220);
    __delay_ms(120);

    play_tone(600, 160);
    __delay_ms(80);

    play_tone(900, 120);

    buzzer_off();
}

unsigned char pr1_active(void)
{
    return (PORTAbits.RA0 == PR_ACTIVE_LEVEL);
}

unsigned char pr2_active(void)
{
    return (PORTAbits.RA1 == PR_ACTIVE_LEVEL);
}

static unsigned char capture_digit_generic(unsigned char (*sensor_active_func)(void))
{
    unsigned char count = 0;
    unsigned int idle_time = 0;

    while (1)
    {
        if (system_halted)
            return 0;

        if (sensor_active_func())
        {
            count++;

            if (count > MAX_INPUT_DIGIT)
                count = MAX_INPUT_DIGIT;

            sevenseg_show(count);

            delay_ms_block(SENSOR_DEBOUNCE_MS);

            while (sensor_active_func())
            {
                if (system_halted)
                    return 0;
            }

            idle_time = 0;
        }
        else
        {
            __delay_ms(10);
            idle_time += 10;

            if ((count > 0) && (idle_time >= DIGIT_DONE_TIMEOUT_MS))
            {
                break;
            }
        }
    }

    return count;
}

unsigned char capture_digit_from_pr1(void)
{
    return capture_digit_generic(pr1_active);
}

unsigned char capture_digit_from_pr2(void)
{
    return capture_digit_generic(pr2_active);
}

void wrong_code_alarm(void)
{
    buzzer_on();
    delay_ms_block(WRONG_CODE_BUZZ_MS);
    buzzer_off();
}

void correct_code_action(void)
{
    relay_on();
    delay_ms_block(MOTOR_ON_TIME_MS);
    relay_off();
}

void system_init(void)
{
    // Clock
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
    TRISBbits.TRISB0 = 1;   // S1
    TRISBbits.TRISB1 = 1;   // Emergency switch

    // Outputs
    TRISCbits.TRISC1 = 0;   // Relay
    TRISCbits.TRISC2 = 0;   // Buzzer
    TRISEbits.TRISE1 = 0;   // LED1 / SYS_LED
    TRISD = 0x00;           // 7-segment

    // Initial states
    relay_off();
    buzzer_off();
    led_on();
    sevenseg_blank();

    // RB1 interrupt-on-change
    // Assumes switch press pulls RB1 LOW
    WPUBbits.WPUB1 = 1;
    IOCBNbits.IOCBN1 = 1;
    IOCBPbits.IOCBP1 = 0;
    IOCBFbits.IOCBF1 = 0;

    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;
    INTCON0bits.GIE = 1;
}

// Emergency interrupt on RB1 using IOC
void __interrupt(irq(IOC), base(8)) emergency_ISR(void)
{
    if (PIR0bits.IOCIF)
    {
        if (IOCBFbits.IOCBF1)
        {
            emergency_flag = 1;
            system_halted = 1;

            relay_off();
            sevenseg_blank();

            // melody must be in ISR
            play_emergency_melody();

            IOCBFbits.IOCBF1 = 0;
        }

        PIR0bits.IOCIF = 0;
    }
}

void main(void)
{
    unsigned char digit1;
    unsigned char digit2;

    system_init();

    while (1)
    {
        if (system_halted)
        {
            relay_off();
            buzzer_off();
            sevenseg_blank();

            while (1)
            {
                // stop forever after emergency
            }
        }

        // First digit from PR1
        digit1 = capture_digit_from_pr1();

        if (system_halted)
            continue;

        sevenseg_show(digit1);
        delay_ms_block(1000);

        // Second digit from PR2
        digit2 = capture_digit_from_pr2();

        if (system_halted)
            continue;

        sevenseg_show(digit2);
        delay_ms_block(1000);

        // Compare with hardcoded secret code
        if ((digit1 == SECRET_DIGIT1) && (digit2 == SECRET_DIGIT2))
        {
            correct_code_action();
        }
        else
        {
            wrong_code_alarm();
        }

        sevenseg_blank();
        delay_ms_block(500);
    }
}
