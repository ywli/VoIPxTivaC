/*
 * ui.c -- user interface
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI library */
#include "tm4c123gh6pm.h"

void ledInit(void)
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;// activate clock for PortF
    while ((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R5) == 0)
    {};                                     // wait until PortF is ready
    GPIO_PORTF_LOCK_R = 0x4C4F434B;         // unlock GPIO PortF
    GPIO_PORTF_CR_R = 0x1F;                 // allow changes to PF4-0
    GPIO_PORTF_AMSEL_R = 0x00;              // disable analog on PortF
    GPIO_PORTF_PCTL_R = 0x00000000;         // use PF4-0 as GPIO
    GPIO_PORTF_DIR_R |= 0x02;               // PF2 output
    GPIO_PORTF_AFSEL_R = 0x00;              // disable alt function on PF
    GPIO_PORTF_DEN_R |= 0x02;               // enable digital I/O on PF4-0
}

void ledOn(void)
{
    GPIO_PORTF_DATA_R |= 0x02;
}

void ledOff(void)
{
    GPIO_PORTF_DATA_R &= ~0x02;
}

void ledToggle(void)
{
	GPIO_PORTF_DATA_R ^= 0x02;
}
