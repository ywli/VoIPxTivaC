// TI library
#include "tm4c123gh6pm.h"
#include "sys.h"
#include "cbuff_dma.h"
#include "dmaBuffer.h"
#include <stdint.h>


#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#define WIFI_BUFFER_SIZE        1024
#define WIFI_TX_BUFFER_SIZE     2048
#define WIFI_TRANSPORT_SIZE     64
#define WIFI_DMA_CH_RX          18
#define WIFI_DMA_CH_TX          19
#define WIFI_PKT_NUM_OF         5

typedef struct{
    int wifiPktSize;
    uint8_t *wifiPktP;
}wifiPktBuffer_t;

struct wifiControlBlock
{
    cbuff_dma_t wifiTxBuffer;
    cbuff_dma_t wifiRxBuffer;
    //dmaBuffer_t wifiTxPktBuffer;
};

struct wifiControlBlock wifiCb;

/* buffer */
uint8_t wifiTxArray[WIFI_TX_BUFFER_SIZE];
uint8_t wifiRxArray[WIFI_BUFFER_SIZE];
wifiPktBuffer_t wifiTxPkt[WIFI_PKT_NUM_OF];


uint8_t wifiAtTest0[] = "AT+CIPMUX=1\r\n";
uint8_t wifiAtTest1[] = "AT+CIPSTART=4,\"UDP\",\"192.168.4.2\",56853\r\n";
uint8_t wifiAtTest2[] = "AT+CIPSEND=4,332\r\n";
uint8_t wifiAtTest3[332];

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
int wifitest = 0;
static void wifiTxDma(void)
{
    uint8_t dmaChId = WIFI_DMA_CH_TX;
    uint8_t *addr;
    int32_t len;

    /* test */
    wifitest++;

    /* get data from buffer */
    if (cbuff_dma_dequeue_driver1(
        &wifiCb.wifiTxBuffer,
        (unsigned char **) &addr,
        &len) == 0)
    {
        /* no data to send */
        return;
    }
    
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

static void wifiTxDma2(
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

void wifiTestInit(void)
{
    /* AT command 1 */
    wifiTxPkt[0].wifiPktP = &wifiAtTest0[0];
    wifiTxPkt[0].wifiPktSize = 13;

    /* AT command 2 */
    wifiTxPkt[1].wifiPktP = &wifiAtTest1[0];
    wifiTxPkt[1].wifiPktSize = 41;

    /* AT command 3 */
    wifiTxPkt[2].wifiPktP = &wifiAtTest2[0];
    wifiTxPkt[2].wifiPktSize = 18;

    /* AT command 4 */
    wifiTxPkt[3].wifiPktP = &wifiAtTest3[0];
    wifiTxPkt[3].wifiPktSize = 332;

    int16_t *dstSampleP = (int16_t *) &wifiAtTest3[12];
    int i;

	/* 8ksps, 1011Hz, 8sp one cycle, max amp.: 32k */
	static const uint16_t spkTestData[] = 
	{
		0x3e80, 0x6ab2, 0x7d00, 0x6ab2, 0x3e80, 0x124e, 0x0000, 0x124e
	};
	for (i = 0; i<160; i++)
	{
		//dstSampleP[i] = spkTestData[(i * 3 ) % 8];//3k tone
		dstSampleP[i] = spkTestData[(i * 1 ) % 8];//1k tone
	}
}

void wifiTestBg(void)
{
    static int i = 0;
    int pktIdx;

    /* determine pktIdx */
    if ((i % 1000) == 0)
    {
        pktIdx = 0;
    }
    else if((i % 1000) == 1)
    {
        pktIdx = 1;
    }
    else if ((i % 2) == 0)
    {
        pktIdx = 2;
    }
    else if ((i % 2) == 1)
    {
        pktIdx = 3;
    }
    i++;

    /* perform DMA transfer on specified packet */
    wifiTxDma2(
        wifiTxPkt[pktIdx].wifiPktP, 
        wifiTxPkt[pktIdx].wifiPktSize);
}

void wifiBackgroundTask(void)
{
    /* TX */
    #if TEST
    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_TX)) == 0)
    {
        /* transfer completed */
        if (wifiCb.wifiTxBuffer.numOfXfer > 0)
        {
            /* confirm block transferred */
            cbuff_dma_dequeue_driver2(&wifiCb.wifiTxBuffer);
        }
        /* idle */
        else
        {
            /* initia next transfer */
            wifiTxDma();
        }
    }
    #else
    if ((UDMA_ENASET_R & (1 << WIFI_DMA_CH_TX)) == 0)
    {
        wifiTestBg();
    }
    #endif
    
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

int wifiInit(void)
{
    uint8_t dmaChId;

    /* initialize control block */
    cbuff_dma_init(
        &wifiCb.wifiTxBuffer,
        &wifiTxArray[0],
        WIFI_TX_BUFFER_SIZE,
        WIFI_TRANSPORT_SIZE);

    cbuff_dma_init(
        &wifiCb.wifiRxBuffer,
        &wifiRxArray[0],
        WIFI_BUFFER_SIZE,
        WIFI_TRANSPORT_SIZE);

    // dmaBufferInit(
    //     &wifiCb.wifiTxPktBuffer,
    //     (unsigned char*) &wifiTxPkt[0],
    //     sizeof(wifiTxPkt),
    //     sizeof(wifiTxPkt[0]));

    #if 1
    wifiTestInit();
    #endif
    
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

    return 0;
}

int wifiRead(
    uint8_t *dataP,
    uint16_t dataLen)
{
    return cbuff_dma_dequeue_app(
                &wifiCb.wifiRxBuffer, 
                dataP, 
                dataLen);
}

int wifiWrite(
    uint8_t *dataP,
    uint16_t dataLen)
{
    /* put data to DMA buffer */
    if (cbuff_dma_enqueue_app(
        &wifiCb.wifiTxBuffer,
        dataP,
        dataLen) == 0)
    {
        /* error */
        //return -1;
    }

    return 0;
}

/* establish a UDP connection */
int wifiConnect(void)
{
    /* AT command 1 */
    wifiWrite("AT+CIPMUX=1\r\n", 13);

    /* delay */
    vTaskDelay(100);
    
    /* AT command 2 */
    wifiWrite("AT+CIPSTART=4,\"UDP\",\"192.168.4.2\",56853\r\n",41);

    /* delay */
    vTaskDelay(100);

    return 0;
}

/* send a UDP packet */
int wifiPktSend(
    unsigned char *dataP,
    int dataLen)
{
    #if 0
    unsigned char *test = "hello\r\n";

    wifiWrite("AT+CIPSEND=4,7\r\n", 16);

    vTaskDelay(10);

    wifiWrite(test, 7);

    vTaskDelay(10);

    #else
    /* AT command 1 */
    wifiWrite("AT+CIPSEND=4,332\r\n", 18);

    /* delay */
    vTaskDelay(20);

    /* send data */
    wifiWrite(dataP, dataLen);

    /* delay */
    vTaskDelay(20);
    #endif 

    return 0;
}
