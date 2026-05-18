/* 
 * File:   FindingLED.h
 * Author: duyph
 *
 * Created on May 17, 2026, 11:29 PM
 */

// CONFIG1L
#pragma config FEXTOSC = LP        // Use low-power external oscillator setting
#pragma config RSTOSC = EXTOSC     // Use external oscillator as the reset/default clock source

// CONFIG1H
#pragma config CLKOUTEN = OFF      // Disable clock output pin
#pragma config PR1WAY = ON         // PRLOCK can be changed only once
#pragma config CSWEN = ON          // Allow clock switching
#pragma config FCMEN = ON          // Enable fail-safe clock monitor

// CONFIG2L
#pragma config MCLRE = EXTMCLR     // Enable external MCLR reset pin
#pragma config PWRTS = PWRT_OFF    // Disable power-up timer
#pragma config MVECEN = OFF        // Disable multi-vector interrupts so simple ISR works
#pragma config IVT1WAY = ON        // Interrupt vector table lock can be changed once
#pragma config LPBOREN = OFF       // Disable low-power brown-out reset
#pragma config BOREN = SBORDIS     // Enable brown-out reset; software control ignored

// CONFIG2H
#pragma config BORV = VBOR_2P45    // Brown-out reset voltage set to 2.45V
#pragma config ZCD = OFF           // Disable zero-cross detect module
#pragma config PPS1WAY = ON        // PPS registers can be unlocked/locked once
#pragma config STVREN = ON         // Reset if stack overflow/underflow happens
#pragma config DEBUG = OFF         // Disable debugger mode
#pragma config XINST = OFF         // Disable extended instruction set

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31  // Watchdog timer postscaler setting
#pragma config WDTE = OFF          // Disable watchdog timer

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7   // Watchdog window always open
#pragma config WDTCCS = SC         // Watchdog clock source controlled by software

// CONFIG4L
#pragma config BBSIZE = BBSIZE_512 // Boot block size is 512 words
#pragma config BBEN = OFF          // Disable boot block
#pragma config SAFEN = OFF         // Disable storage area flash
#pragma config WRTAPP = OFF        // Application flash is not write-protected

// CONFIG4H
#pragma config WRTB = OFF          // Boot block is not write-protected
#pragma config WRTC = OFF          // Configuration registers are not write-protected
#pragma config WRTD = OFF          // EEPROM is not write-protected
#pragma config WRTSAF = OFF        // Storage area flash is not write-protected
#pragma config LVP = ON            // Enable low-voltage programming

// CONFIG5L
#pragma config CP = OFF            // Disable code protection

#ifndef FINDINGLED_H
#define	FINDINGLED_H

#include <xc.h>
#define _XTAL_FREQ 4000000UL

// =====================================================
// USER SETTINGS
// =====================================================

// Fixed flat ADC value.
// For Vref = 5V and accelerometer zero-g voltage around 1.65V:
// ADC = (1.65 / 5.0) * 4096 = about 1352
#define FIXED_CENTER_ADC       1352U
#define TILT_DELTA_COUNTS      60U
#define SHAKE_DELTA_COUNTS     200U
#define SAMPLE_COUNT           26U
#define MOVE_DELAY_MS         350U
#define BUZZER LATCbits.LATC2
#define INVERT_DIRECTION       1 // Change to 1 if left/right direction is backwards
#define LED_ACTIVE_LOW         0 // Change to 1 if your LED bar turns ON with logic LOW
#define TRIAL_LED1  LATCbits.LATC4
#define TRIAL_LED2  LATCbits.LATC5
#define TRIAL_LED3  LATCbits.LATC6

// =====================================================
// VARIABLES
// =====================================================

volatile uint8_t confirmRequest = 0;

volatile uint16_t adc_value = 0;
volatile uint16_t avg_adc = 0;
volatile uint16_t min_adc = 0;
volatile uint16_t max_adc = 0;
volatile uint16_t swing_adc = 0;
volatile int16_t tilt_adc = 0;

volatile uint8_t led_position = 3;
volatile uint8_t secret_position = 0;
volatile uint8_t attempts_left = 3;
volatile uint8_t gameFinished = 0;
volatile uint8_t randomSeed = 0;

// state values for debugging in MPLAB Watch
// 0 = flat
// 1 = tilt left
// 2 = tilt right
// 3 = shake reset
// 4 = correct
// 5 = wrong
// 6 = game over
volatile uint8_t state = 0;

// =====================================================
// FUNCTION PROTOTYPES
// =====================================================

void ADC_Init(void);
uint16_t ADC_Read(void);
uint8_t i;

void LED_Init(void);
void LED_ResetCenter(void);
void LED_ShowPosition(uint8_t position);
void LED_MoveLeft(void);
void LED_MoveRight(void);

void TrialLED_Init(void);
void UpdateTrialLEDs(void);

void Buzzer_Init(void);
void Buzzer_Beep_ms(uint16_t ms);
void Buzzer_ResetSound(void);

void Button_Init(void);
void Button_WaitRelease(void);

void StartNewGame(void);
void CheckGuess(void);
void WinAction(void);
void WrongAction(void);
void GameOverAction(void);

void Delay_ms(uint16_t ms);

// =====================================================
// GAME FUNCTIONS
// =====================================================

void StartNewGame(void)
{
    gameFinished = 0;
    attempts_left = 3;
    state = 0;

    UpdateTrialLEDs();
    LED_ResetCenter();

    adc_value = ADC_Read();
    randomSeed ^= (uint8_t)adc_value;
    randomSeed += (uint8_t)ADRESL;

    secret_position = (uint8_t)((ADRESL ^ ADRESH ^ randomSeed ) & 0x07);
    
    LATD = 0xFF;
    Delay_ms (120);
    LATD = 0x00;
    Delay_ms (120);
    LED_ResetCenter();
}

void CheckGuess(void)
{
    if(led_position == secret_position)
    {
        state = 4;
        WinAction();
    }
    else
    {
        state = 5;

        if(attempts_left > 0)
        {
            attempts_left--;
        }

        UpdateTrialLEDs();

        if(attempts_left == 0)
        {
            state = 6;
            GameOverAction();
        }
        else
        {
            WrongAction();
        }
    }
}

void WinAction(void)
{
    gameFinished = 1;
    for(i = 0; i < 10; i++)
    {
        LATD = 0xFF;      
        BUZZER = 1;       
        Delay_ms(100);

        LATD = 0x00;     
        BUZZER = 0;      
        Delay_ms(100);
    }
    LATD = 0xFF;
    BUZZER = 1;
    Delay_ms(1500);

    BUZZER = 0;
    Delay_ms(150);
}

void WrongAction(void)
{
    BUZZER = 1;
    for(i = 0; i < 4; i++) // wrong led blink
    {
        LATD = 0x81;
        Delay_ms(150);
        LATD = 0x00;
        Delay_ms(150);
    }
    BUZZER = 0;
    LED_ShowPosition(led_position);
}

void GameOverAction(void)
{   
    gameFinished = 1;
    BUZZER = 1;
    for(i = 0; i < 15; i++) // wrong led blink
    {
        LATD = 0xAA;
        Delay_ms(150);
        LATD = 0x00;
        LATD = 0x55;
        Delay_ms(150);
        LATD = 0x00;
    }
    BUZZER = 0;
    LED_ShowPosition(led_position);
}


// ADC FUNCTIONS
// =====================================================

void ADC_Init(void)
{
    // RA1 / ANA1 = accelerometer analog input
    TRISAbits.TRISA1 = 1;
    ANSELAbits.ANSELA1 = 1;

    ADCON0bits.FM = 1;      // Right justify ADC result
    ADCON0bits.CS = 1;      // ADCRC clock

    ADPCH = 0x01;           // Select ANA1 / RA1

    ADREFbits.PREF = 0;     // Vref+ = VDD
    ADREFbits.NREF = 0;     // Vref- = VSS

    ADCLK = 0x00;

    ADRESH = 0x00;
    ADRESL = 0x00;

    ADPREL = 0x00;
    ADPREH = 0x00;

    ADACQL = 0x20;
    ADACQH = 0x00;

    ADCON0bits.ON = 1;      // ADC ON
}

uint16_t ADC_Read(void)
{
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);
    return ((uint16_t)ADRESH << 8) | ADRESL;
}

// =====================================================
// LED BAR FUNCTIONS: RD0-RD7
// =====================================================

void LED_Init(void)
{
    ANSELD = 0x00;      // PORTD digital
    TRISD = 0x00;       // PORTD output
    LATD = 0x00;
}

void LED_ResetCenter(void)
{
    led_position = 3;
    LED_ShowPosition(led_position);
}

void LED_ShowPosition(uint8_t position)
{
    if(position > 7)
    {
        position = 7;
    }
    LATD = (uint8_t)(1 << position);
}

void LED_MoveLeft(void)
{
    if(led_position > 0)
    {
        led_position--;
    }
    LED_ShowPosition(led_position);
}

void LED_MoveRight(void)
{
    if(led_position < 7)
    {
        led_position++;
    }
    LED_ShowPosition(led_position);
}

// =====================================================
// TRIAL LED FUNCTIONS: RC4, RC5, RC6
// =====================================================
void TrialLED_Init(void)
{
    ANSELCbits.ANSELC4 = 0;
    ANSELCbits.ANSELC5 = 0;
    ANSELCbits.ANSELC6 = 0;

    TRISCbits.TRISC4 = 0;
    TRISCbits.TRISC5 = 0;
    TRISCbits.TRISC6 = 0;

    TRIAL_LED1 = 0;
    TRIAL_LED2 = 0;
    TRIAL_LED3 = 0;
}

void UpdateTrialLEDs(void)
{
    TRIAL_LED1 = (attempts_left >= 1) ? 1 : 0;
    TRIAL_LED2 = (attempts_left >= 2) ? 1 : 0;
    TRIAL_LED3 = (attempts_left >= 3) ? 1 : 0;
}

// =====================================================
// ACTIVE BUZZER FUNCTIONS: RC2
// =====================================================
void Buzzer_Init(void)
{
    ANSELCbits.ANSELC2 = 0;   // RC2 digital
    TRISCbits.TRISC2 = 0;     // RC2 output
    LATCbits.LATC2 = 0;       // buzzer off
}

void Buzzer_Beep_ms(uint16_t ms)
{
    BUZZER = 1;
    Delay_ms(ms);
    BUZZER = 0;
}

void Buzzer_ResetSound(void)
{
    Buzzer_Beep_ms(120);
    Delay_ms(80);
    Buzzer_Beep_ms(120);
}

// =====================================================
// CONFIRM BUTTON FUNCTIONS: RB3
// =====================================================
void Button_Init(void)
{
    ANSELBbits.ANSELB3 = 0; // RB3 = confirm button input
    TRISBbits.TRISB3 = 1;
   
    WPUBbits.WPUB3 = 1; // Enable weak pull-up on RB3
    
    IOCBFbits.IOCBF3 = 0; // Clear IOC flag

    IOCBPbits.IOCBP3 = 0; // Falling edge interrupt:
    IOCBNbits.IOCBN3 = 1;

    PIE0bits.IOCIE = 1; // Enable IOC interrupt
    INTCON0bits.IPEN = 0;   // Disable interrupt priority for simple interrupt mode
    INTCON0bits.GIE = 1;    // Enable global interrupts
}

void Button_WaitRelease(void)
{
    // Simple debounce and wait for release
    Delay_ms(50);
    while(PORTBbits.RB3 == 0)
    {
        // wait until button is released
    }
    Delay_ms(80);
    IOCBFbits.IOCBF3 = 0; // Clear any bounce flag that happened during release
    PIE0bits.IOCIE = 1; // Re-enable IOC after the button is fully released
}

// =====================================================
// DELAY FUNCTION
// =====================================================
void Delay_ms(uint16_t ms)
{
    while(ms > 0)
    {
        __delay_ms(1);
        ms--;
    }
}

// =====================================================
// INTERRUPT SERVICE ROUTINE
// Use this style if config has MVECEN = OFF
// =====================================================
void __interrupt() ISR(void)
{
    // Button IOC interrupt
if(PIR0bits.IOCIF && PIE0bits.IOCIE)
{
    if(IOCBFbits.IOCBF3)
    {
        if(PORTBbits.RB3 == 0) // Confirm only if button is pressed low
        {
            confirmRequest = 1;
            PIE0bits.IOCIE = 0; // Disable IOC until the main code handles this press
        }
        IOCBFbits.IOCBF3 = 0;
    }
}
}

#endif	/* FINDINGLED_H */


