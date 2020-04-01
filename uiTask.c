/*
 * tx.c -- transmit task
 *
 *
 */

/* standard library */
#include <stdint.h>

/* FreeRTOS includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

/* project resources */
#include "ui.h"

/** 
 * UI task
 * param: pvParameters -> task argument
 * return: none
**/
void uiTask(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(1000);
		uiLedToggle();
    }
}
