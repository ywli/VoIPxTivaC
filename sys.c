/*
 * sys.c -- system module
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resources */
#include "common.h"
#include "dma.h"
#include "sys.h"
#include "ui.h"
#include "testMode.h"

/** 
 * Initialize PLL
 * param: none
 * return: none
**/
static int sysPllInit(void)
{
	/* 
	 * system clock to 80 MHz
	 */

	// 0) configure the system to use RCC2 for advanced features
	//    such as 400 MHz PLL and non-integer System Clock Divisor
	SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;
	// 1) bypass PLL while initializing
	SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;
	// 2) select the crystal value and oscillator source
	SYSCTL_RCC_R &= ~SYSCTL_RCC_XTAL_M;   // clear XTAL field
	SYSCTL_RCC_R += SYSCTL_RCC_XTAL_16MHZ;// configure for 16 MHz crystal
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;// clear oscillator source field
	SYSCTL_RCC2_R += SYSCTL_RCC2_OSCSRC2_MO;// configure for main oscillator source
	// 3) activate PLL by clearing PWRDN
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;
	// 4) set the desired system divider and the system divider least significant bit
	SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400;  // use 400 MHz PLL
	SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~0x1FC00000) // clear system clock divider field
					+ (4<<22);      // configure for 80 MHz clock
	// 5) wait for the PLL to lock by polling PLLLRIS
	while((SYSCTL_RIS_R&SYSCTL_RIS_PLLLRIS)==0){};
	// 6) enable use of PLL by clearing BYPASS
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;

	// /* 1- bypass PLL and system clock divider */
	// SYSCTL_RCC_R |= SYSCTL_RCC_BYPASS;
	// SYSCTL_RCC_R &= ~SYSCTL_RCC_USESYSDIV;

	// /* 2- */

	// /* 3- */

	// /* 4- wait for PLL to lock */
	// while((SYSCTL_RIS_R & SYSCTL_RIS_PLLLRIS) ==0 )
	// {};

	// /* 5- enable PLL */
	// SYSCTL_RCC_R &= ~SYSCTL_RCC_BYPASS;
	// SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;

	return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Initialize microphone IO pins
 * param: none
 * return: none
**/
static int sysPinMicInit(void)
{
	/* 
	 * GPIO Microphone config 
	 * used pins: 
	 *   J3.09, PE3, AIN0
	 */
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
    while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R) == 0)// wait until ready
    {};

	/* step 3 set GPIO PE3 alternative function mode, p. 821  */
	GPIO_PORTE_AFSEL_R |= (1<<3);

	/* step 4 disable PE3 digital function, p.682 */
	GPIO_PORTE_DEN_R |= ~(1<<3);

	/* step 5 enable PE3 analog function, p.687 */
	GPIO_PORTE_AMSEL_R |= (1<<3);

	return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Initialize speaker IO pins
 * param: none
 * return: none
**/
static int sysPinSpkInit(void)
{
	/*
	 * GPIO Speaker config
	 * used pins:
	 *   J1.06, PE5, M0PWM5, Bit Clock
	 *   J1.07, PB4, M0PWM2, Frame Clock I2S
	 *   J1.02, PB5, M0PWM3, Frame Clock SPI
	 *   J1.08, PA5, SSI0TX, Data bit, pull-down
	 *   J2.09, PA3, SSI0FSS
	 *   J2.10, PA2, SSI0CLK
	 */

	/* 2- enable clock to gpio module */
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1|
	                     SYSCTL_RCGCGPIO_R4|
						 SYSCTL_RCGCGPIO_R0;
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1) == 0)// wait until PortB is ready
    {};                                     
	while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0)// wait until PortE is ready
    {};                                     
	while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0) == 0)// wait until PortA is ready
    {};

	/* 3- gpio gpioafsel register config */
	GPIO_PORTE_AFSEL_R |= (1<<5);// PE5
	GPIO_PORTB_AFSEL_R |= (1<<4);// PB4
	GPIO_PORTB_AFSEL_R |= (1<<5);// PB5
	GPIO_PORTA_AFSEL_R |= (1 << 5)|
	                      (1 << 3)|
						  (1 << 2);

	/* 4- gpioctl register config */
	GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE5_M0PWM5;//PE5
	GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB4_M0PWM2;//PB4
	GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB5_M0PWM3;//PB5
	GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA5_SSI0TX|
						 GPIO_PCTL_PA3_SSI0FSS|
						 GPIO_PCTL_PA2_SSI0CLK;

	/* 5- enable digital */
	GPIO_PORTE_DEN_R |= (1<<5);//PE5
	GPIO_PORTB_DEN_R |= (1<<4);//PB4
	GPIO_PORTB_DEN_R |= (1<<5);//PB5
	GPIO_PORTA_DEN_R |= (1 << 5)|
	                    (1 << 3)|
						(1 << 2);

	/* pull down PA5 */
	GPIO_PORTA_PDR_R |= (1 << 5);

	return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Initialize Wifi IO pins
 * param: none
 * return: none
**/
static int sysPinWifiInit(void)
{
	/* 
	 * Wifi config 
	 * used pins: 
	 *   J4.04, PC4, U4RX
	 *   J4.05, PC5, U4TX
	 */
	/* 1- enable UART module */
	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R4;
    while ((SYSCTL_RCGCUART_R & SYSCTL_RCGCUART_R4) == 0)// wait until ready
    {};
	/* 2- enable clock to gpio module */
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R2) == 0)// wait until ready
    {};     
	/* 3- gpio gpioafsel register config */
	GPIO_PORTC_AFSEL_R |= (1 << 4)|
	                      (1 << 5);

	/* 4- gpioctl register config */
	GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_U4RX|
						 GPIO_PCTL_PC5_U4TX;

	/* 5- enable digital */
	GPIO_PORTC_DEN_R |= (1 << 4)|
	                    (1 << 5);

	/* pull-up */
	GPIO_PORTC_PUR_R |= (1 << 4)|
	                    (1 << 5);

	return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Initialize UI IO pins
 * param: none
 * return: none
**/
static int sysPinUiInit(void)
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

	return COMMON_RETURN_STATUS_SUCCESS;
}

extern int spkInit(void);
/** 
 * Initialize system resources
 * param: none
 * return: none
**/
int sysInit(void)
{
	sysPllInit();

	/* DMA */
	dmaInit();
	
	/*
	 * ADC 
	 */

	/* step 1 enable clock to ADC module 0 */
	SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
	while ((SYSCTL_PRADC_R & SYSCTL_PRADC_R0) ==0 )
	{};
	/* enable timer0 */
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
	while ((SYSCTL_PRTIMER_R & SYSCTL_PRTIMER_R0) == 0)
	{};

	/*
	 * GPIO Speaker config
	 * used pins:
	 *   J1.06, PE5, M0PWM5, Bit Clock
	 *   J1.07, PB4, M0PWM2, Frame Clock
	 *   J1.08, PA5, SSI0TX, Data bit, pull-down
	 *   J2.09, PA3, SSI0FSS
	 *   J2.10, PA2, SSI0CLK
	 */
	sysPinSpkInit();

	/* 1- enable ssi0 module */
	SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R0;
	while ((SYSCTL_RCGCSSI_R & SYSCTL_RCGCSSI_R0) == 0)
	{};

	/* 1- enable PWM clock */
	SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
	while ((SYSCTL_RCGCPWM_R & SYSCTL_RCGCPWM_R0) == 0)
	{};

	/* 5- PWM clock is system clock */
	SYSCTL_RCC_R &= ~SYSCTL_RCC_USEPWMDIV;

	spkInit();

	
	/* 
	 * Wifi config 
	 * used pins: 
	 *   J4.04, PC4, U4RX
	 *   J4.05, PC5, U4TX
	 */
	sysPinWifiInit();

	/* 
	 * GPIO Microphone config 
	 * used pins: 
	 *   J3.09, PE3, AIN0
	 */
	sysPinMicInit();

	/* UI */
	sysPinUiInit();

	/* test mode resource */
	SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R5;
	while ((SYSCTL_RCGCWTIMER_R & SYSCTL_RCGCWTIMER_R5) == 0)
	{};

	testModeTimerInit();

	return COMMON_RETURN_STATUS_SUCCESS;
}
