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

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 4000000UL

// =====================================================
// Hardware setup
// =====================================================
// Accelerometer analog output connected to RA1 / ANA1
// LED bar connected to RD0-RD7

// =====================================================
// ADC / motion settings based on Vref = 5V
// =====================================================
// 12-bit ADC range: 0 to 4095
// 0.10V = 0.10 / 5.0 * 4096 = about 82 counts
// 0.35V = 0.35 / 5.0 * 4096 = about 287 counts

#define TILT_DELTA_COUNTS    82U
#define SHAKE_DELTA_COUNTS   287U

#define SAMPLE_COUNT         32U
#define MOVE_DELAY_MS        25U

// Change this to 1 if left/right direction is backwards
#define INVERT_DIRECTION     1

// Change this to 1 if your LED bar is active-low
#define LED_ACTIVE_LOW       0

// If 1, the system measures flat position at startup
#define AUTO_CALIBRATE       1

// If AUTO_CALIBRATE = 0, use this fixed flat value.
// For ZERO_G = 1.65V and Vref = 5V:
// 1.65 / 5.0 * 4096 = about 1352
#define FIXED_CENTER_ADC     1352U

// =====================================================
// Global variables
// =====================================================

volatile uint16_t adc_value = 0;
volatile uint16_t adc_center = 0;
volatile uint16_t avg_adc = 0;
volatile uint16_t min_adc = 0;
volatile uint16_t max_adc = 0;
volatile uint16_t swing_adc = 0;
volatile int16_t tilt_adc = 0;

volatile uint8_t led_position = 3;

// state:
// 0 = flat
// 1 = tilt left
// 2 = tilt right
// 3 = shake reset
volatile uint8_t state = 0;

// =====================================================
// Function prototypes
// =====================================================

void ADC_Init(void);
uint16_t ADC_Read(void);
uint16_t ADC_Average(void);

void LED_Init(void);
void LED_Write(uint8_t pattern);
void LED_ResetCenter(void);
void LED_ShowPosition(uint8_t position);
void LED_MoveLeft(void);
void LED_MoveRight(void);

// =====================================================
// Main program
// =====================================================

void main(void)
{
    uint32_t sum;
    uint8_t i;

    ADC_Init();
    LED_Init();

    // Start with center LEDs on
    LED_ResetCenter();

    // Wait for accelerometer to settle
    __delay_ms(1000);

#if AUTO_CALIBRATE
    // Keep the accelerometer flat during startup.
    // This measured value becomes the flat reference.
    adc_center = ADC_Average();
#else
    adc_center = FIXED_CENTER_ADC;
#endif

    while(1)
    {
        sum = 0;
        min_adc = 4095;
        max_adc = 0;

        // Take multiple samples
        for(i = 0; i < SAMPLE_COUNT; i++)
        {
            adc_value = ADC_Read();

            sum += adc_value;

            if(adc_value < min_adc)
            {
                min_adc = adc_value;
            }

            if(adc_value > max_adc)
            {
                max_adc = adc_value;
            }

            __delay_ms(2);
        }

        avg_adc = (uint16_t)(sum / SAMPLE_COUNT);
        swing_adc = max_adc - min_adc;

        tilt_adc = (int16_t)avg_adc - (int16_t)adc_center;

#if INVERT_DIRECTION
        tilt_adc = -tilt_adc;
#endif

        // =================================================
        // Motion logic
        // =================================================

        // Shake reset
        if(swing_adc > SHAKE_DELTA_COUNTS)
        {
            state = 3;
            LED_ResetCenter();
            __delay_ms(400);
        }

        // Flat: stop LED motion
        else if((tilt_adc > -(int16_t)TILT_DELTA_COUNTS) &&
                (tilt_adc <  (int16_t)TILT_DELTA_COUNTS))
        {
            state = 0;

            // Do nothing here.
            // LED stays where it is when flat.
            __delay_ms(MOVE_DELAY_MS);
        }

        // Tilt left
        else if(tilt_adc < -(int16_t)TILT_DELTA_COUNTS)
        {
            state = 1;
            LED_MoveLeft();
            __delay_ms(MOVE_DELAY_MS);
        }

        // Tilt right
        else
        {
            state = 2;
            LED_MoveRight();
            __delay_ms(MOVE_DELAY_MS);
        }
    }
}

// =====================================================
// ADC functions
// =====================================================

void ADC_Init(void)
{
    // RA1 / ANA1 as analog input
    TRISAbits.TRISA1 = 1;
    ANSELAbits.ANSELA1 = 1;

    ADCON0bits.FM = 1;      // Right justify ADC result
    ADCON0bits.CS = 1;      // Use ADCRC clock

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

    ADCON0bits.ON = 1;      // Turn ADC on
}

uint16_t ADC_Read(void)
{
    ADCON0bits.GO = 1;          // Start ADC conversion

    while(ADCON0bits.GO);       // Wait until conversion is complete

    return ((uint16_t)ADRESH << 8) | ADRESL;
}

uint16_t ADC_Average(void)
{
    uint32_t sum = 0;
    uint8_t i;

    for(i = 0; i < SAMPLE_COUNT; i++)
    {
        sum += ADC_Read();
        __delay_ms(5);
    }

    return (uint16_t)(sum / SAMPLE_COUNT);
}

// =====================================================
// LED bar functions
// =====================================================

void LED_Init(void)
{
    ANSELD = 0x00;      // PORTD digital
    TRISD = 0x00;       // RD0-RD7 outputs
    LATD = 0x00;
}

void LED_Write(uint8_t pattern)
{
#if LED_ACTIVE_LOW
    LATD = (uint8_t)(~pattern);
#else
    LATD = pattern;
#endif
}

void LED_ResetCenter(void)
{
    led_position = 3;

    // Center LEDs ON: RD3 and RD4
    LED_Write(0b00011000);
}

void LED_ShowPosition(uint8_t position)
{
    if(position > 7)
    {
        position = 7;
    }

    LED_Write((uint8_t)(1 << position));
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
