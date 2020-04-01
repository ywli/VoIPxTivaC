/*
 * ui.c -- user interface
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI library */
#include "tm4c123gh6pm.h"

/** 
 * Switch on UI LED
 * param: none
 * return: none
**/
void uiLedOn(void)
{
    GPIO_PORTF_DATA_R |= 0x02;
}

/** 
 * Switch off UI LED
 * param: none
 * return: none
**/
void uiLedOff(void)
{
    GPIO_PORTF_DATA_R &= ~0x02;
}

/** 
 * Toggle UI LED
 * param: none
 * return: none
**/
void uiLedToggle(void)
{
	GPIO_PORTF_DATA_R ^= 0x02;
}
