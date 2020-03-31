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
#include "queue.h"

/* project resources */
#include "ui.h"

// Flash the LEDs on the launchpad
void uiTask(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(1000);
		ledToggle();
    }
}
