
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


void txInit(void)
{
    /* initialize mic module */
    micInit();

    /* initialize wifi module */
    wifiInit();

    /* initialize rtp module */
    rtpInit();
}

int micBlockFilter(
	int16_t input[], 
	int16_t output[], 
	int num);

void txLoop()
{
    wifiXferBlock_t *wifiPktP;
    micDataBlock_t *audioBlockP;

    /* 
    * read audio block 
    */
    audioBlockP = micBlockGet();
    /* no data */
    if (audioBlockP == 0)
    {
        return;
    }

    /* apply filter */
    micBlockFilter(
        &audioBlockP->micDataBlock[0],
        &audioBlockP->micDataBlock[0],
        MIC_BLOCK_NUM_OF_SP);

    /* initialize buffer */
    wifiPktP = wifiPktSend1();
    if (wifiPktP == 0)
    {
        return;
    }
    /* send RTP packet over wifi */
    wifiPktSend2();

    /* 
    * generate RTP packet 
    */
    wifiPktP->wifiPktSize = rtpWrite(
                wifiPktP->wifiPktP,
                &audioBlockP->micDataBlock[0],
                MIC_BLOCK_NUM_OF_SP);
    if (wifiPktP->wifiPktSize <= 0)
    {
        /* error tbd */
        return;
    }
    
    return;
}

/* from microphone to Wifi */
void txTask(void *pvParameters)
{
    for (;;)
    {
        txLoop();
    }
}

