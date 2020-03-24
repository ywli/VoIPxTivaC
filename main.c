/* FreeRTOS 10 Tiva Demo
 *
 * main.c
 *
 * Andy Kobyljanec
 *
 * This is a simple demonstration project of FreeRTOS 8.2 on the Tiva Launchpad
 * EK-TM4C1294XL.  TivaWare driverlib sourcecode is included.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "mic.h"
#include "sys.h"
#include "ui.h"
#include "wifi.h"
#include "spk.h"

// TI library
#include "tm4c123gh6pm.h"


// Demo Task declarations
void demoLEDTask(void *pvParameters);
void txTask(void *pvParameters);


// Flash the LEDs on the launchpad
void demoLEDTask(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(1000);
		ledToggle();
		//printf("hello\n");
    }
}


/* from microphone to Wifi */
void txTask(void *pvParameters)
{
    //uint16_t block[160];
    for (;;)
    {
		// if (micReadBlock(&block[0]) == 0)
        // {
        // }
		vTaskDelay(10);
    }
}
uint16_t data[160];

// Main function
int main(void)
{
    // Create demo tasks
	sysInit();
	ledInit();
	micInit();
    wifiInit();
    spkInit();
    
    // spkWrite(
    //     &data[0],
    //     160);

    //wifiWrite("01234567890123456789", 20);
    //wifiWrite("AT\n", 3);
    wifiWrite("AT+GMR\r\n", 8);
    

    xTaskCreate(demoLEDTask, (const portCHAR *)"LEDs",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(txTask, (const portCHAR *)"Tx",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    //Code should never reach this point
    
    return 0;
}
