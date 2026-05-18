/* ---------------------
 * Title: Finding the Secret LED 
 * ---------------------
 * Program Details:
 * The purpose of this program is to create a tilt-controlled secret LED
 * guessing game using 3 features of PIC18F46K42: ADC input, 
 * Interrupt-On-Change, digital output, and visual/audio feedback.
 
 * The accelerometer uses the ADC module to read input. The 8-red-LED bar 
 * shows the current selected LED position. The confirm button uses
 * Interrupt-On-Change to detect the button press. Three green LEDs show the 
 * number of attempts remaining. 
 * 
 * Date: May 18, 2026
 * File Dependencies / Libraries: It is required to include the 
 * Configuration Header File (FindingLED.h)
 * Compiler: xc8, 2.4
 * 
 * Author: Duy Pham
 */

#include <xc.h>
#include "FindingLED.h"

// =====================================================
// MAIN PROGRAM
// =====================================================

void main(void)
{
    uint32_t sum;
    
    ADC_Init();
    LED_Init();
    TrialLED_Init();
    Buzzer_Init();
    Button_Init();
    StartNewGame();

    while(1)
{
    if(confirmRequest) // Confirm button was pressed
    {
        confirmRequest = 0;
        Button_WaitRelease();
        if(!gameFinished)
        {
            CheckGuess();
        }
    }
    
    sum = 0;
    min_adc = 4095;
    max_adc = 0;
    
    for(i = 0; i < SAMPLE_COUNT; i++) // Read accelerometer several times
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

    tilt_adc = (int16_t)avg_adc - (int16_t)FIXED_CENTER_ADC;

#if INVERT_DIRECTION
    tilt_adc = -tilt_adc;
#endif

    // Improve random seed using ADC movement/noise
    randomSeed ^= (uint8_t)adc_value;
    randomSeed += (uint8_t)ADRESL;

    // Shake to reset at anytime
    if(swing_adc > SHAKE_DELTA_COUNTS)
    {
        state = 3;
        StartNewGame();
        Buzzer_ResetSound();
        Delay_ms(300);
    }

    // If game is finished, wait for shake reset
    else if(gameFinished)
    {
        // Do nothing until shake is detected
        Delay_ms(MOVE_DELAY_MS);
    }

    // Flat mode: LED stays where it is
    else if((tilt_adc > -(int16_t)TILT_DELTA_COUNTS) &&
            (tilt_adc <  (int16_t)TILT_DELTA_COUNTS))
    {
        state = 0;

        // Do nothing.
        // LED keeps current position.
        Delay_ms(MOVE_DELAY_MS);
    }

    // Tilt left
    else if(tilt_adc < -(int16_t)TILT_DELTA_COUNTS)
    {
        state = 1;
        LED_MoveLeft();
        Delay_ms(MOVE_DELAY_MS);
    }

    // Tilt right
    else
    {
        state = 2;
        LED_MoveRight();
        Delay_ms(MOVE_DELAY_MS);
    }
}
}
