/**
 * @file    Leds.h
 * @date    22 Sep 2024
 * @brief   File containing LED, Nucleo board-focused function prototypes.
 *
 * This library provides an interface for controlling the LEDs on the UCSC
 * Nucleo I/O Shield development board.
 */

#ifndef LEDS_H
#define LEDS_H

/**
 * LEDs_Init() Initializes the LED bar by doing two things:
 *      1) Use the appropriate SFRs to set each LED pin to "output" mode.
 *      2) Use the appropriate SFRs to set each LED pin's output value to 0 (low
 *         voltage). 
 * After calling LEDs_Init(), the other functions in this file can be used to 
 * manipulate the LED bar.
 */
void LEDs_Init(void);

/**
 * LEDs_Set() controls the output on the LED bar.  Input values are 8-bit 
 * patterns, where each bit describes the desired state of the corresponding 
 * LED. So, for example, LEDs_Set(0x80) should  leave the first LED on, and the
 * last 7 LEDs off. LEDs_Set(0x1F) should turn off the first three LEDs and turn
 * on the remaining five LEDs. 
 *
 * @param newPattern:  An 8-bit value describing the desired output on the LED
 * bar. 
 *
 * LEDs_Set should not change any LED pins to inputs.
 */
void LEDs_Set(char newPattern);

/**
 * LEDs_Get() returns the current state of the LED bar.  Return values are 8-bit
 * patterns, where each bit describes the current state of the corresponding
 * LED. So, for example, if the LED bar's LED's are
 *
 * [ON OFF ON OFF   OFF ON OFF ON],
 *
 * LEDs_Get() should return 0xA5.
 * 
 * @return char:  An 8-bit value describing the current output on the LED bar.  
 * 
 * LEDs_Get() should not change the state of the LEDs, or any SFRs.
 */
char LEDs_Get(void);

#endif // LEDS_H
