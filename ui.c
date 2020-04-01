/*
 * ui.c -- user interface
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI library */
#include "tm4c123gh6pm.h"

void uiLedOn(void)
{
    GPIO_PORTF_DATA_R |= 0x02;
}

void uiLedOff(void)
{
    GPIO_PORTF_DATA_R &= ~0x02;
}

void uiLedToggle(void)
{
	GPIO_PORTF_DATA_R ^= 0x02;
}
