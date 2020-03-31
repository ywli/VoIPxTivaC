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
#include "common.h"
#include "wifi.h"

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
