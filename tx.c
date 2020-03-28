
/* standard C library */
#include <stdint.h>
//#include <stdbool.h>
#include <stdio.h>

/* FreeRTOS includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* modules */
#include "mic.h"
#include "wifi.h"
#include "rtp.h"


typedef struct
{
    uint8_t txPktBuffer[512];
    uint16_t txPktSize;
}txControlBlock_t;

txControlBlock_t txCb;

void txInit(void)
{
    /* initialize mic module */
    micInit();

    /* initialize wifi module */
    wifiInit();

    /* initialize rtp module */
    rtp_init();
}

/* from microphone to Wifi */
void txTask(void *pvParameters)
{
    int16_t *sampleP;
    uint16_t sampleLen;
    uint8_t *pktP;
    uint16_t pktLen;
    int res;
    int cnt = 0;
    
    for (;;)
    {
        /* periodically setup wifi */
        if (((cnt++) % 1000) == 0)
        {
            wifiConnect();
            vTaskDelay(100);
        }

        /* initialize buffer */
        pktLen = 0;
        pktP = &txCb.txPktBuffer[0];
        
        /* read block from Mic */
        sampleP = (int16_t *) micReadBlock();

        /* generate RTP packet */
        #if 1
        if ((res = rtp_write(
                pktP,
                sampleP,
                160)) <= 0)
        {
            /* error tbd */
            continue;
        }
        pktLen += res;
        #else
        pktLen = 332;
        #endif
        
        /* send RTP packet over wifi */
        wifiPktSend(
            pktP, 
            pktLen);

    }
}

