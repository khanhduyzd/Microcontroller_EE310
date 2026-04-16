#include "config_words.h"
#include "init_values.h"
#include "functions.h"

volatile unsigned char emergency_flag = 0;

// 7-segment patterns
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

// ==========================
// LED control
// ==========================
void update_confirm_led(void)
{
    if (PORTBbits.RB0 == 0)   // pressed
        LATEbits.LATE1 = LED_ACTIVE_LEVEL;
    else
        LATEbits.LATE1 = !LED_ACTIVE_LEVEL;
}

// ==========================
// Basic controls
// ==========================
void relay_on(void)  { LATCbits.LATC1 = RELAY_ACTIVE_LEVEL; }
void relay_off(void) { LATCbits.LATC1 = !RELAY_ACTIVE_LEVEL; }

void buzzer_on(void)  { LATCbits.LATC2 = BUZZER_ACTIVE_LEVEL; }
void buzzer_off(void) { LATCbits.LATC2 = !BUZZER_ACTIVE_LEVEL; }

void sevenseg_show(unsigned char digit)
{
    unsigned char pattern = seg_table[digit];

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

// ==========================
// Delay helpers
// ==========================
void delay_variable_us(unsigned int us)
{
    unsigned int i;
    for (i = 0; i < us; i++)
        __delay_us(1);
}

void delay_ms_block(unsigned int ms)
{
    unsigned int i;
    for (i = 0; i < ms; i++)
        __delay_ms(1);
}

// ==========================
// Buzzer tones
// ==========================
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
    play_tone(900, 120); __delay_ms(80);
    play_tone(600, 160); __delay_ms(80);
    play_tone(350, 220); __delay_ms(120);
    play_tone(600, 160); __delay_ms(80);
    play_tone(900, 120);

    buzzer_off();
}

// ==========================
// Sensor helpers
// ==========================
unsigned char pr1_active(void)
{
    return (PORTAbits.RA0 == PR_ACTIVE_LEVEL);
}

unsigned char pr2_active(void)
{
    return (PORTAbits.RA1 == PR_ACTIVE_LEVEL);
}

unsigned char confirm_pressed(void)
{
    return (PORTBbits.RB0 == 0);
}

// ==========================
// Digit capture using confirm switch
// ==========================
static unsigned char capture_digit_generic(unsigned char (*sensor)(void))
{
    unsigned char count = 0;

    while (1)
    {
        update_confirm_led();

        if (emergency_flag)
            return 0;

        if (sensor())
        {
            count++;
            if (count > MAX_INPUT_DIGIT)
                count = MAX_INPUT_DIGIT;

            sevenseg_show(count);
            delay_ms_block(SENSOR_DEBOUNCE_MS);

            while (sensor())
            {
                update_confirm_led();
                if (emergency_flag)
                    return 0;
            }
        }

        if (confirm_pressed())
        {
            delay_ms_block(50);

            while (confirm_pressed())
            {
                update_confirm_led();
                if (emergency_flag)
                    return 0;
            }

            return count;
        }

        __delay_ms(10);
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

// ==========================
// Actions
// ==========================
void wrong_code_alarm(void)
{
    unsigned int i;

    buzzer_on();
    for (i = 0; i < WRONG_CODE_BUZZ_MS; i++)
    {
        if (emergency_flag)
        {
            buzzer_off();
            return;
        }
        __delay_ms(1);
    }
    buzzer_off();
}

void correct_code_action(void)
{
    unsigned int i;

    relay_on();
    for (i = 0; i < MOTOR_ON_TIME_MS; i++)
    {
        if (emergency_flag)
        {
            relay_off();
            return;
        }
        __delay_ms(1);
    }
    relay_off();
}

// ==========================
// Initialization
// ==========================
void system_init(void)
{
    OSCCON1 = 0x60;
    OSCFRQ  = 0x00;

    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = 0;

    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;

    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISEbits.TRISE1 = 0;
    TRISD = 0x00;

    relay_off();
    buzzer_off();
    sevenseg_blank();

    WPUBbits.WPUB0 = 1;
    WPUBbits.WPUB1 = 1;

    IOCBNbits.IOCBN1 = 1;
    IOCBFbits.IOCBF1 = 0;

    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;
    INTCON0bits.GIE = 1;
}

// ==========================
// ISR
// ==========================
void __interrupt(irq(IOC), base(8)) emergency_ISR(void)
{
    if (PIR0bits.IOCIF)
    {
        if (IOCBFbits.IOCBF1)
        {
            PIE0bits.IOCIE = 0;

            emergency_flag = 1;

            relay_off();
            sevenseg_blank();

            play_emergency_melody();

            while (PORTBbits.RB1 == 0);

            __delay_ms(50);

            IOCBFbits.IOCBF1 = 0;
            PIR0bits.IOCIF = 0;

            PIE0bits.IOCIE = 1;
        }
    }
}

// ==========================
// MAIN
// ==========================
void main(void)
{
    unsigned char d1, d2;

    system_init();

    while (1)
    {
        update_confirm_led();

        if (emergency_flag)
        {
            emergency_flag = 0;
            relay_off();
            buzzer_off();
            sevenseg_blank();
            continue;
        }

        d1 = capture_digit_from_pr1();
        if (emergency_flag) continue;

        sevenseg_show(d1);
        delay_ms_block(500);

        d2 = capture_digit_from_pr2();
        if (emergency_flag) continue;

        sevenseg_show(d2);
        delay_ms_block(500);

        if ((d1 == SECRET_DIGIT1) && (d2 == SECRET_DIGIT2))
            correct_code_action();
        else
            wrong_code_alarm();

        sevenseg_blank();
        delay_ms_block(500);
    }
}
