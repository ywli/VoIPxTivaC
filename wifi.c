/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resource */
#include "sys.h"
#include "cbuff_dma.h"
#include "dmaBuffer.h"
#include "wifi.h"

#define WIFI_BUFFER_SIZE        1024
#define WIFI_TRANSPORT_SIZE     64
#define WIFI_DMA_CH_RX          18
#define WIFI_DMA_CH_TX          19
#define WIFI_PKT_NUM_OF         4
#define WIFI_PKT_SIZE           172

struct wifiControlBlock
{
    cbuff_dma_t wifiRxBuffer;
    dmaBuffer_t wifiTxPktBuffer;
};
struct wifiControlBlock wifiCb;

/* buffer */
uint8_t wifiRxArray[WIFI_BUFFER_SIZE];

/* TX packet */
wifiXferBlock_t wifiTxPkt[WIFI_PKT_NUM_OF];
uint8_t wifiPkt0[WIFI_PKT_SIZE];
uint8_t wifiPkt1[WIFI_PKT_SIZE];
uint8_t wifiPkt2[WIFI_PKT_SIZE];
uint8_t wifiPkt3[WIFI_PKT_SIZE];

/* AT command */
wifiXferBlock_t wifiAt[3];
uint8_t wifiAtTest0[] = "AT+CIPMUX=1\r\n";
uint8_t wifiAtTest1[] = "AT+CIPSTART=4,\"UDP\",\"192.168.4.2\",56853\r\n";
uint8_t wifiAtTest2[] = "AT+CIPSEND=4,172\r\n";

static void wifiDmaInit(void)
{
    uint8_t dmaChId;

    /* 
	 * set a DMA channel for receive 
	 */

	/* channel attribute */
	dmaChId = WIFI_DMA_CH_RX;

	/* configure channel assignment, CH18, enc 2 = UART4RX */
	UDMA_CHMAP2_R &= ~UDMA_CHMAP2_CH18SEL_M;
	UDMA_CHMAP2_R |= ((2 << UDMA_CHMAP2_CH18SEL_S) & UDMA_CHMAP2_CH18SEL_M);

	/* 1- default channel priority level, omitted */

	/* 2- use primary control structure, omitted */

	/* 3- respond to both single and burst requests, omitted */
	
	/* 4- clear mask */
	UDMA_REQMASKCLR_R |= (1 << dmaChId);

	/* channel control structure */
    wifiRxDma();


    /* 
	 * set a DMA channel for transmit 
	 */

	/* channel attribut */
	dmaChId = WIFI_DMA_CH_TX;

	/* configure channel assignment, CH19, enc 2 = UART4TX */
	UDMA_CHMAP2_R &= ~UDMA_CHMAP2_CH19SEL_M;
	UDMA_CHMAP2_R |= ((2 << UDMA_CHMAP2_CH19SEL_S) & UDMA_CHMAP2_CH19SEL_M);

	/* 1- default channel priority level, omitted */

	/* 2- use primary control structure, omitted */

	/* 3- respond to both single and burst requests, omitted */
	
	/* 4- clear mask */
	UDMA_REQMASKCLR_R |= (1 << dmaChId);

    return;
}

static void wifiRxDma(void)
{
    uint8_t dmaChId;
    uint8_t *addr;
    int32_t len;

    dmaChId = WIFI_DMA_CH_RX;

    if (UDMA_CHIS_R & (1 << dmaChId) == 0)
    {
        return;
    }

    /* clear uDMA status bit */
    UDMA_CHIS_R |= (1 << dmaChId);

    /* prepare space in DMA buffer */
    cbuff_dma_enqueue_driver1(
        &wifiCb.wifiRxBuffer, 
        &addr, 
        &len);

    /* DMA channel control description */
    dmaTable[dmaChId].dstAddr = (uint32_t) (addr + len - 1);
	dmaTable[dmaChId].srcAddr = (uint32_t) &UART4_DR_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_8  |
							  UDMA_CHCTL_DSTSIZE_8 |
							  UDMA_CHCTL_SRCINC_NONE|
							  UDMA_CHCTL_SRCSIZE_8 |
							  UDMA_CHCTL_ARBSIZE_1  |//revise
							  (((len - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
                              UDMA_CHCTL_XFERMODE_BASIC;

    /* re-enable DMA */
    UDMA_ENASET_R |= (1 << WIFI_DMA_CH_RX);
}

static void wifiTxDma(
    uint8_t *addr,
    int32_t len)
{
    uint8_t dmaChId = WIFI_DMA_CH_TX;
    
    /* still running */
    if (UDMA_ENASET_R & (1 << dmaChId))
    {
        return;
    }

    /* UART still full */
    if (UART4_FR_R & UART_FR_TXFF)
    {
        return;
    }

    /* clear DMA status bit */
    UDMA_CHIS_R |= (1 << dmaChId);

    /* DMA channel control description */
    dmaTable[dmaChId].srcAddr = (uint32_t) (addr + len - 1);
	dmaTable[dmaChId].dstAddr = (uint32_t) &UART4_DR_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_NONE|
							  UDMA_CHCTL_DSTSIZE_8  |
							  UDMA_CHCTL_SRCINC_8   |
							  UDMA_CHCTL_SRCSIZE_8  |
							  UDMA_CHCTL_ARBSIZE_1  |//revise
							  (((len - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
                              UDMA_CHCTL_XFERMODE_BASIC;

    /* re-enable DMA */
    UDMA_ENASET_R |= (1 << dmaChId);
}

static void wifiTxPktInit(void)
{
    /* initialize packet buffer */
    dmaBufferInit(
        &wifiCb.wifiTxPktBuffer,
        (unsigned char *) &wifiTxPkt[0],
        sizeof(wifiTxPkt),
        sizeof(wifiTxPkt[0]));
    
    /* initialize element in packet buffer */
    wifiTxPkt[0].wifiPktP = &wifiPkt0[0];
    wifiTxPkt[0].wifiPktSize = sizeof(wifiPkt0);

    wifiTxPkt[1].wifiPktP = &wifiPkt1[0];
    wifiTxPkt[1].wifiPktSize = sizeof(wifiPkt1);

    wifiTxPkt[2].wifiPktP = &wifiPkt2[0];
    wifiTxPkt[2].wifiPktSize = sizeof(wifiPkt2);

    wifiTxPkt[3].wifiPktP = &wifiPkt3[0];
    wifiTxPkt[3].wifiPktSize = sizeof(wifiPkt3);

    #if 0
    int i;
    for (i = 0; i < wifiTxPkt[0].wifiPktSize; i++)
    {
        wifiTxPkt[0].wifiPktP[i] = 'a';
    }
    #endif
}

static void wifiAtInit(void)
{
    /* AT command 0 */
    wifiAt[0].wifiPktP = &wifiAtTest0[0];
    wifiAt[0].wifiPktSize = 13;

    /* AT command 1 */
    wifiAt[1].wifiPktP = &wifiAtTest1[0];
    wifiAt[1].wifiPktSize = 41;

    /* AT command 2 */
    wifiAt[2].wifiPktP = &wifiAtTest2[0];
    wifiAt[2].wifiPktSize = 18;
}

int wifiTxBackgroundTask(void)
{
    static int i = 0;
    wifiXferBlock_t *pktP = (wifiXferBlock_t *) 0;

    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_TX)) != 0)
    {
        return WIFI_STATUS_FAILURE;
    }

    /* determine c */
    if ((i % 1000) == 0)
    {
        /* AT command 0 */
        pktP = &wifiAt[0];
    }
    else if((i % 1000) == 1)
    {
        /* AT command 1 */
        pktP = &wifiAt[1];
    }
    else// if (wifiCb.wifiTxPktBuffer.numOfAvail > 0)
    {
        if ((i % 2) == 0)
        {
            /* AT command 2 */
            pktP = &wifiAt[2];
        }
        else if ((i % 2) == 1)
        {
            /* packet */
            pktP = (wifiXferBlock_t *) dmaBufferGet(
                        &wifiCb.wifiTxPktBuffer, 
                        DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);

            if (pktP != 0)
            {
                dmaBufferGet(
                    &wifiCb.wifiTxPktBuffer, 
                    DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);
            }
            pktP = &wifiTxPkt[0];
        }
    }
    

    if (pktP == 0)
    {
        return WIFI_STATUS_FAILURE;
    }

    /* perform DMA transfer */
    wifiTxDma(
        pktP->wifiPktP, 
        pktP->wifiPktSize);

    i++;
    return WIFI_STATUS_SUCCESS;
}

int wifiTest = 0;
int wifiTxBackgroundTask2(int c)
{
    wifiXferBlock_t *pktP = (wifiXferBlock_t *) 0;

    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_TX)) != 0)
    {
        return WIFI_STATUS_FAILURE;
    }

    /* determine c */
    if ((c == 0) || (c == 1))
    {
        /* AT command 0 and 1 */
        pktP = &wifiAt[c];
    }
    else if (wifiCb.wifiTxPktBuffer.numOfAvail > 0)
    {
        if (c == 2)
        {
            pktP = &wifiAt[2];
        }
        else
        {
            /* packet */
            pktP = (wifiXferBlock_t *) dmaBufferGet(
                        &wifiCb.wifiTxPktBuffer, 
                        DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);

            if (pktP != 0)
            {
                dmaBufferGet(
                    &wifiCb.wifiTxPktBuffer, 
                    DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);
            }
        }
    }
    else
    {
        return WIFI_STATUS_FAILURE;
    }

    if (pktP == 0)
    {
        wifiTest++;
        return WIFI_STATUS_FAILURE;
    }

    /* perform DMA transfer */
    wifiTxDma(
        pktP->wifiPktP, 
        pktP->wifiPktSize);

    return WIFI_STATUS_SUCCESS;
}

static void wifiUartInit(void)
{
    /* UART config */
    /* 1- disable UART */
    UART4_CTL_R = 0;

    /* 2-, 3- baudrate config */
    UART4_IBRD_R = 43;
    UART4_FBRD_R = 26;

    /* 4- serial parameter */
    UART4_LCRH_R = UART_LCRH_WLEN_8|
                   UART_LCRH_FEN;

    /* 5- UART clock config system clock */
    UART4_CC_R = 0x00;

    /* 6- DMA config */
    UART4_DMACTL_R = UART_DMACTL_RXDMAE|
                     UART_DMACTL_TXDMAE;

    /* enable interrupt */
    // UART4_IM_R = UART_IM_TXIM|
    //              UART_IM_RXIM;

    // NVIC_EN1_R |= (1<<28);


    /* 7- enable UART */
    UART4_CTL_R = UART_CTL_UARTEN|
                  UART_CTL_RXE|
                  UART_CTL_TXE;
}

void wifiBackgroundTask(void)
{
    /* TX */
    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_TX)) == 0)
    {
        //wifiTxBackgroundTask();
    }
    
    /* RX */
    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_RX)) == 0)
    {
        /* transfer completed */
        if (wifiCb.wifiRxBuffer.numOfXfer > 0)
        {
            /* confirm block transferred */
            cbuff_dma_enqueue_driver2(&wifiCb.wifiRxBuffer);
        }
        /* idle */
        else
        {
            /* initia next transfer */
            wifiRxDma();
        }
    }
}

/** 
 * Initialize module 
 * param: none
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiInit(void)
{
    /* initialize control block */
    cbuff_dma_init(
        &wifiCb.wifiRxBuffer,
        &wifiRxArray[0],
        WIFI_BUFFER_SIZE,
        WIFI_TRANSPORT_SIZE);

    /* initialize Tx packet buffer */
    wifiTxPktInit();

    /* initialize AT commands */
    wifiAtInit();
    
    /* initialize UART */
    wifiUartInit();

    /* initialize DMA */
    wifiDmaInit();

    return WIFI_STATUS_SUCCESS;
}

/** 
 * Read data from Wifi module
 * param: dataP -> output data reference
 * param: dataLen -> output data size in bytes
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiRead(
    uint8_t *dataP,
    uint16_t dataLen)
{
    return cbuff_dma_dequeue_app(
                &wifiCb.wifiRxBuffer, 
                dataP, 
                dataLen);
}

/** 
 * Send packet (get packet reference)
 * param: none
 * return: (wifiXferBlock_t*) -> packet reference
**/
wifiXferBlock_t* wifiPktSend1(void)
{
    return dmaBufferPut(
            &wifiCb.wifiTxPktBuffer,
            DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1);
}

/** 
 * Send packet (actual send the written packet, 
 *             returned by wifiPktSend1)
 * param: none
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiPktSend2(void)
{
    dmaBufferPut(
        &wifiCb.wifiTxPktBuffer, 
        DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_2);

    return WIFI_STATUS_SUCCESS;
}
