// **** Include libraries here ****
// Standard libraries.
#include <stdio.h>

// Course library.
#include <BOARD.h>

// **** Set any macros or preprocessor directives here ****


// **** Set any local typedefs here ****
typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING
} OvenState;

typedef struct {
    OvenState state;
    //add more members to this struct
} OvenData;

// **** Declare any datatypes here ****

// **** Define any module-level, global, or external variables here ****
// Create a local variable to update with ADC readings.
volatile static uint32_t adc_val = 0;
volatile uint8_t hasNewADC = FALSE;


// **** Put any helper functions here ****


/* This function will update your OLED to reflect the state. */
void updateOvenOLED(OvenData ovenData){
    // Update OLED here.
}

/* This function will execute your state machine.  
 * It should ONLY run if an event flag has been set.
 */
void runOvenSM(void)
{
    // Write your SM logic here.
}


int main()
{
    BOARD_Init();

    printf(
        "Welcome to CRUZID's Lab08 (Toaster Oven)."
        "Compiled on %s %s.\n\r",
        __TIME__,
        __DATE__
    );

    // Initialize state machine (and anything else you need to init) here.

    while(1) {
        // Add main loop code here:
        // check for events
        // on event, run runOvenSM()
        // clear event flags
    };
}


/**
 * This is the interrupt for the Timer2 peripheral. It should check for button
 * events and store them in a module-level variable.
 * 
 * You should not modify this function for ButtonsTest.c or bounce_buttons.c!
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    /***************************************************************************
     * Your code goes in between this comment and the following one with 
     * asterisks.
     **************************************************************************/
    if (htim == &htim4) {
        // TIM4 interrupt.
    } else if (htim == &htim3) {
        // TIM3 interrupt.
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
    adc_val = HAL_ADC_GetValue (&hadc1);
    // Set the flag to indicate a new value is available.
    hasNewADC = TRUE;
    // Start the next conversion.
    HAL_ADC_Start_IT(&hadc1);
}
