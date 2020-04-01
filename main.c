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

/** 
 * Initialize main
 * param: none
 * return: none
**/
void mainInit(void)
{
    sysInit();
    txInit();

    return;
}

/** 
 * main function
 * param: none
 * return: (int) -> execution status
**/
int main(void)
{
    /* initialize */
	mainInit();

    xTaskCreate(uiTask, (const portCHAR *)"UI",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(txTask, (const portCHAR *)"TX",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(hwTask, (const portCHAR *)"HW",
                configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    //Code should never reach this point
    
    return 0;
}
