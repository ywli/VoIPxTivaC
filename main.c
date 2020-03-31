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
#include "sys.h"
#include "txTask.h"
#include "hwTask.h"
#include "uiTask.h"

void mainInit(void)
{
    sysInit();
    txInit();

    return;
}

int main(void)
{
    /* initialize */
	mainInit();

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
