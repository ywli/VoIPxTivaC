/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resource */
#include "sys.h"
#include "cbuff_dma.h"
#include "dmaBuffer.h"
#include "dma.h"
#include "wifi.h"

#define WIFI_RX_BLOCK_SIZE      32
#define WIFI_RX_BLOCK_NUM_OF    5

#define WIFI_DMA_CH_RX          18
#define WIFI_DMA_CH_TX          19
#define WIFI_PKT_NUM_OF         4
#define WIFI_PKT_SIZE           172

struct wifiControlBlock
{
    dmaBuffer_t wifiRxBuffer;
    dmaBuffer_t wifiTxPktBuffer;
};
struct wifiControlBlock wifiCb;

/* buffer */
uint8_t wifiRxArray[WIFI_RX_BLOCK_SIZE * WIFI_RX_BLOCK_NUM_OF];

/* TX packet */
wifiXferBlock_t wifiTxPkt[WIFI_PKT_NUM_OF];
uint8_t wifiPkt0[WIFI_PKT_SIZE];
uint8_t wifiPkt1[WIFI_PKT_SIZE];
uint8_t wifiPkt2[WIFI_PKT_SIZE];
uint8_t wifiPkt3[WIFI_PKT_SIZE];

/* TX test packet */
#if WIFI_TEST_CANNED_TX_PKT
wifiXferBlock_t wifiTxTestPkt[1];
uint8_t wifiTxTestPkt0[WIFI_PKT_SIZE];
#endif 

/* AT command */
wifiXferBlock_t wifiAt[3];
uint8_t wifiAtTest0[] = "AT+CIPMUX=1\r\n";
uint8_t wifiAtTest1[] = "AT+CIPSTART=4,\"UDP\",\"192.168.4.2\",56853\r\n";
uint8_t wifiAtTest2[] = "AT+CIPSEND=4,172\r\n";

static void wifiDmaRxRequest(void)
{
    uint8_t *addr;
    int32_t len;

    if (UART4_FR_R & UART_FR_RXFE)
    {
        return;
    }
    
    /* prepare space in DMA buffer */
    addr = (uint8_t *) dmaBufferPut(
        &wifiCb.wifiRxBuffer,
        DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_1);
    if (addr == 0)
    {
        //tbd: error
        return;
    }

    dmaBufferPut(
        &wifiCb.wifiRxBuffer,
        DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_2);
    len = WIFI_RX_BLOCK_SIZE;

    dmaChRequest(
        WIFI_DMA_RX_CH,
        &UART4_DR_R,
        addr,
        len);
}

static void wifiDmaTxRequest(
    uint8_t *addr,
    int32_t len)
{
    /* UART still full */
    if (UART4_FR_R & UART_FR_TXFF)
    {
        return;
    }

    dmaChRequest(
        WIFI_DMA_TX_CH, 
        addr, 
        &UART4_DR_R, 
        len);
}

static void wifiDmaInit(void)
{
    /* set a DMA channel for transmit */
    dmaChInit(
        WIFI_DMA_TX_CH,
        WIFI_DMA_TX_ENC,
        WIFI_DMA_TX_MODE,
        WIFI_DMA_TX_DIR,
        WIFI_DMA_TX_ELEMENT_SIZE);

    /* set a DMA channel for receive */
    dmaChInit(
        WIFI_DMA_RX_CH,
        WIFI_DMA_RX_ENC,
        WIFI_DMA_RX_MODE,
        WIFI_DMA_RX_DIR,
        WIFI_DMA_RX_ELEMENT_SIZE);

	/* channel control structure */
    wifiDmaRxRequest();

    return;
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

    #if WIFI_TEST_CANNED_TX_PKT
    int i;
    for (i = 0; i < sizeof(wifiTxTestPkt0); i++)
    {
        wifiTxTestPkt0[i] = '0' + (i %10);
    }
    wifiTxTestPkt[0].wifiPktP = &wifiTxTestPkt0[0];
    wifiTxTestPkt[0].wifiPktSize = sizeof(wifiTxTestPkt0);
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

int wifiRxBackgroundTask(void)
{
    //tbd
    wifiDmaRxRequest();
}

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
            #if WIFI_TEST_CANNED_TX_PKT
            pktP = &wifiTxTestPkt[0];
            #endif
        }
    }
    else
    {
        return WIFI_STATUS_FAILURE;
    }

    if (pktP == 0)
    {
        return WIFI_STATUS_FAILURE;
    }

    /* perform DMA transfer */
    wifiDmaTxRequest(
        pktP->wifiPktP, 
        pktP->wifiPktSize);

    wifiDmaRxRequest();

    return WIFI_STATUS_SUCCESS;
}

/** 
 * Initialize module 
 * param: none
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiInit(void)
{
    /* initialize control block */
    dmaBufferInit(
        &wifiCb.wifiRxBuffer, 
        &wifiRxArray[0], 
        sizeof(wifiRxArray), 
        WIFI_RX_BLOCK_SIZE);

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
    //tbd 
    // dmaBufferGet(
    //     &wifiCb.wifiRxBuffer,
    //     DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);

    // dmaBufferGet(
    //     &wifiCb.wifiRxBuffer,
    //     DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);
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
