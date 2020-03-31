/*
 * main.c -- 
 *
 *
 */

/* standard library */

/* FreeRTOS includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* project resources */
#include "common.h"
#include "sys.h"
#include "ui.h"
#include "spk.h"
#include "tx.h"
#include "wifi.h"

// Demo Task declarations
void demoLEDTask(void *pvParameters);


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

extern int wifiTxBackgroundTask2(int);
int i = 0;
/* hardware background task */
void hwTask(void *pvParameters)
{
    for (;;)
    {
        if ((i%1000) == 0)
        {
            wifiTxBackgroundTask2(0);
            vTaskDelay(10);
        }
        else if ((i%1000) == 1)
        {
            wifiTxBackgroundTask2(1);
            vTaskDelay(10);
        }
        i++;


        if (wifiTxBackgroundTask2(2) == COMMON_RETURN_STATUS_SUCCESS)
        {
            vTaskDelay(2);
        }
        else
        {
            continue;
        }
        
        if (wifiTxBackgroundTask2(3) == COMMON_RETURN_STATUS_SUCCESS)
        {
            vTaskDelay(16);//it will be busy lower than that 
        }

        
    }
}

// Main function
int main(void)
{
    // Create demo tasks
	sysInit();
	ledInit();
    txInit();

    xTaskCreate(demoLEDTask, (const portCHAR *)"LEDs",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(txTask, (const portCHAR *)"Tx",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(hwTask, (const portCHAR *)"HWBG",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    //Code should never reach this point
    
    return 0;
}
