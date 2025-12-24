// **** Include libraries here ****
// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Course library.
#include <BOARD.h>
#include "Buttons.h"
#include <Timers.h>
#include <Oled.h>
#include <OledDriver.h>
#include <Adc.h>
#include <Ascii.h>
#include "Leds.h"
// **** Set any macros or preprocessor directives here ****

// **** Set any local typedefs here ****
typedef enum
{
    SETUP,
    SELECTOR_CHANGE_PENDING,
    COOKING,
    RESET_PENDING
} OvenState;
typedef enum
{
    BAKE,
    TOAST,
    BROIL
} States;
typedef enum
{
    SELECTOR_TIME = 0,
    SELECTOR_TEMP = 1
} SelectorMode;

typedef struct
{
    OvenState state;
    int mode;          // Cooking mode (e.g., "Bake", "Toast")
    int timeRemaining; // Time remaining (for modes that require time, e.g., Bake)
    int initialTime;   // Set time (for the user to set the desired time)
    int temperature;   // Current temperature
    int selector;
    // add more members to this struct
} OvenData;

// **** Declare any datatypes here ****

// **** Define any module-level, global, or external variables here ****
#define TEMP 350
#define TEMP_BROIL 500
#define LEDS_COOK 0xFF
#define LEDS_OFF 0x00
#define SECOND 1000
#define LONG_PRESS 5
#define ADC_WINDOW_SIZE 20
#define ADC_EDGE_THRESHOLD 3

volatile static uint32_t adc_val = 0;
volatile uint8_t hasNewADC = FALSE;
static volatile uint16_t freeRunningTimer = 0;
static volatile uint16_t buttonTimer = 0;
static uint8_t TimerTick;
static OvenData ovenData;
static uint8_t buttonsEvents = BUTTON_EVENT_NONE;
static uint16_t previousTimer = 0;
static uint16_t nextTimer = 0;
static uint16_t ledFlicker;
static volatile uint8_t currentLED = 0;
static uint32_t adc_window[ADC_WINDOW_SIZE];
static uint8_t adc_window_index = 0;
static uint32_t last_stable_adc_value = 0;

// **** Put any helper functions here ****

/* This function will update your OLED to reflect the state. */
void updateOvenOLED(OvenData ovenData)
{
    char top[6];    // Buffer for 5 heating elements + null terminator
    char bottom[6]; // Buffer for 5 heating elements + null terminator
    const char *topState = (ovenData.state == COOKING || ovenData.state == RESET_PENDING) ? OVEN_TOP_ON : OVEN_TOP_OFF;
    const char *bottomState = (ovenData.state == COOKING || ovenData.state == RESET_PENDING) ? OVEN_BOTTOM_ON : OVEN_BOTTOM_OFF;
    const char *timeArrow = (ovenData.selector == SELECTOR_TIME) ? ">" : " ";
    const char *tempArrow = (ovenData.selector == SELECTOR_TEMP) ? ">" : " ";

    // Repeat the heating element states 5 times for top and bottom
    snprintf(top, sizeof(top), "%s%s%s%s%s", topState, topState, topState, topState, topState);
    snprintf(bottom, sizeof(bottom), "%s%s%s%s%s", bottomState, bottomState, bottomState, bottomState, bottomState);
    
    // Create the display string for different oven modes
    char display[100] = ""; // Buffer to hold the full display string
    switch (ovenData.mode)
    {
    case BAKE:
        sprintf(display, "|%s| MODE: BAKE\n|     |%sTime: %d:%02d\n|-----|%sTemp: %d%sF\n|%s|",
                top, timeArrow, ovenData.timeRemaining / 60, ovenData.timeRemaining % 60, tempArrow, ovenData.temperature, DEGREE_SYMBOL, bottom);
        break;

    case TOAST:
        sprintf(display, "|%s| MODE: TOAST\n|     |>Time: %d:%02d\n|-----|     \n|%s|",
                top, ovenData.timeRemaining / 60, ovenData.timeRemaining % 60, bottom);
        break;

    case BROIL:
        sprintf(display, "|%s| MODE: BROIL\n|     |>Time: %d:%02d\n|-----| Temp: %d%sF\n|%s|",
                top, ovenData.timeRemaining / 60, ovenData.timeRemaining % 60, ovenData.temperature, DEGREE_SYMBOL, bottom);
        break;

    default:
        break;
    }
    // Clear the display again, draw the string, and update the OLED
    OledClear(OLED_COLOR_BLACK);
    OledDrawString(display);
    OledUpdate();
}

/* This function will execute your state machine.
 * It should ONLY run if an event flag has been set.
 */
void runOvenSM(void)
{
    switch (ovenData.state)
    {
    case SETUP:
        if (hasNewADC)
        { // Check if new ADC value has been obtained only Use the top 8 bits
            // Check if in time or temperature setting mode
            if (ovenData.mode == BAKE)
            {
                if (ovenData.selector == SELECTOR_TIME)
                {
                    uint8_t top8Bits = (uint8_t)(adc_val >> 4); // Update cooking time based on ADC value
                    ovenData.timeRemaining = top8Bits + 1;     
                }
                else if (ovenData.selector == 1)
                {
                    // Update cooking temperature based on ADC value (range 300 to 555)
                    uint8_t top8Bits = (uint8_t)(adc_val >> 4);
                    ovenData.temperature = top8Bits + 300;
                }
            }
            else
            {
                uint8_t top8Bits = (uint8_t)(adc_val >> 4);
                ovenData.timeRemaining = top8Bits + 1;
            }
            hasNewADC = FALSE;
            updateOvenOLED(ovenData);
        }
        // Transition to SELECTOR_CHANGE_PENDING when BUTTON_EVENT_3DOWN is detected
        if (buttonsEvents & BUTTON_EVENT_3DOWN)
        {
            ovenData.state = SELECTOR_CHANGE_PENDING;
            previousTimer = freeRunningTimer;
        }
        // Transition to COOKING when BUTTON_EVENT_4DOWN is detected
        if (buttonsEvents & BUTTON_EVENT_4DOWN)
        {
            nextTimer = freeRunningTimer;
            ovenData.initialTime = ovenData.timeRemaining;
            LEDs_Set(LEDS_COOK); // Turn on cooking LEDs
            ledFlicker = (ovenData.timeRemaining + 5) / 8;
            ovenData.state = COOKING; // Switch to cooking state
        }
        break;
    case SELECTOR_CHANGE_PENDING:
        if (buttonsEvents & BUTTON_EVENT_3UP)
        {
            printf("BUTTON_EVENT_3UP detected\n");
            uint16_t elapsedTime = (freeRunningTimer - previousTimer);
            if ((elapsedTime) > LONG_PRESS)
            {
                ovenData.selector ^= 1;
            }
            else
            {
                if (ovenData.mode == BAKE)
                {
                    ovenData.mode = TOAST;
                    ovenData.temperature = 350; // Set temperature for TOAST
                }
                else if (ovenData.mode == TOAST)
                {
                    ovenData.mode = BROIL;
                    ovenData.temperature = 500; // Set temperature for BROIL
                }
                else
                {
                    ovenData.mode = BAKE;
                    ovenData.temperature = 350; // Set temperature for BAKE
                }
            }
            updateOvenOLED(ovenData);
            ovenData.state = SETUP;
        }
        break;
    case COOKING:
        if (TimerTick)
        {
            TimerTick = 0;
            if (ovenData.timeRemaining > 0)
            {
                ovenData.timeRemaining--;
                uint8_t nextLED = ((freeRunningTimer - nextTimer) * 200) / (ledFlicker * 1000);
                currentLED = LEDS_COOK << nextLED; // progress LEDS every 8th of time
                LEDs_Set(currentLED);
            }
            else
            {
                LEDs_Set(LEDS_OFF);
                ovenData.timeRemaining = 1;
                if (ovenData.mode == BAKE)
                {
                    ovenData.temperature = 350; // set bake temp
                }
                else if (ovenData.mode == BROIL)
                {
                    ovenData.temperature = 500; // set broil temp
                }
            }
            if ((freeRunningTimer) % 5 == 0) // set timer decrement for every second
            {
                ovenData.timeRemaining--;
            }
        }

        // Transition to RESET_PENDING if BUTTON_EVENT_4DOWN is detected
        if (buttonsEvents & BUTTON_EVENT_4DOWN)
        {
            nextTimer = freeRunningTimer; // Store button press time
            ovenData.state = RESET_PENDING;
        }
        if (ovenData.timeRemaining == 0) // if out of time set back to initital time and turn off LEDS
        {
            ovenData.timeRemaining = ovenData.initialTime;
            LEDs_Set(LEDS_OFF);
            ovenData.state = SETUP; // go back to setup
        }
        updateOvenOLED(ovenData);
        break;

    case RESET_PENDING:
        if (buttonsEvents & BUTTON_EVENT_4UP)
        {
            uint16_t elapsedTime = (freeRunningTimer - nextTimer);
            if (elapsedTime > LONG_PRESS) // if click is long enough we go back to original settings
            {
                ovenData.timeRemaining = ovenData.initialTime;

                if (ovenData.mode == BAKE)
                {
                    ovenData.temperature = 350;
                }
                else if (ovenData.mode == BROIL)
                {
                    ovenData.temperature = 500;
                }
                LEDs_Set(LEDS_OFF); // turn leds off
                ovenData.state = SETUP; // go back to setup
            }
            else
            {

                ovenData.state = COOKING;
            }
            updateOvenOLED(ovenData);
        }
        break;
    default:
        break;
    }
}

int main()
{
    BOARD_Init();

    printf(
        "Welcome to cksacram's Lab08 (Toaster Oven)."
        "Compiled on %s %s.\n\r",
        __TIME__,
        __DATE__);

    // Initialize state machine (and anything else you need to init) here.
    OledInit();
    ButtonsInit();
    LEDs_Init();
    ADC_Init();
    ovenData.state = SETUP;     // Initial state is SETUP
    ovenData.mode = BAKE;       // Set mode to "Bake"
    ovenData.timeRemaining = 1;
    ovenData.initialTime = 1;
    ovenData.temperature = 350;
    // Update OLED with initial oven data
    updateOvenOLED(ovenData);

    // Enter an infinite loop (in a real program, you'd want to periodically update this display)
    while (1)
    {
        // Add main loop code here:
        // check for events
        // on event, run runOvenSM()
        // clear event flags
        if (hasNewADC)
        {
            runOvenSM();
            hasNewADC = FALSE;
        }
        if (buttonsEvents)
        {
            runOvenSM();
            buttonsEvents = BUTTON_EVENT_NONE;
        }
        if (TimerTick)
        {
            runOvenSM();
            TimerTick = 0;
        }
    }
};

/**
 * This is the interrupt for the Timer2 peripheral. It should check for button
 * events and store them in a module-level variable.
 *
 * You should not modify this function for ButtonsTest.c or bounce_buttons.c!
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /***************************************************************************
     * Your code goes in between this comment and the following one with
     * asterisks.
     **************************************************************************/
    if (htim == &htim4)
    {
        // TIM4 interrupt.
        TimerTick = 1;
        freeRunningTimer++;
    }
    else if (htim == &htim3)
    {
        // TIM3 interrupt.
        if (buttonsEvents == BUTTON_EVENT_NONE)
        {
            buttonsEvents = ButtonsCheckEvents();
        }
    }
    /***************************************************************************
     * Your code goes in between this comment and the preceding one with
     * asterisks.
     **************************************************************************/
}

/**
 * This is the interrupt for the ADC1 peripheral. It will trigger whenever a new
 * ADC reading is available in the ADC buffer
 *
 * It should not be called, and should communicate with main code only by using
 * module-level variables.
 */
void ADC_IRQHandler(void)
{
    // Get the result of the current conversion.
    adc_val = HAL_ADC_GetValue(&hadc1);
    adc_window[adc_window_index] = adc_val;

    // Move the index for the next sample (circular buffer)
    adc_window_index = (adc_window_index + 1) % ADC_WINDOW_SIZE;

    // If the buffer is filled, calculate the average
    if (adc_window_index == 0)
    {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < ADC_WINDOW_SIZE; i++)
        {
            sum += adc_window[i];
        }

        // Calculate the average of the samples in the window
        uint32_t average_adc_value = sum / ADC_WINDOW_SIZE;

        // Only update if the new average is sufficiently different from the last stable value
        if (abs((int)(average_adc_value - last_stable_adc_value)) >= ADC_EDGE_THRESHOLD)
        {
            last_stable_adc_value = average_adc_value;
            hasNewADC = TRUE; // Set the flag that a new stable ADC value is available
        }
    }
    HAL_ADC_Start_IT(&hadc1);
}
