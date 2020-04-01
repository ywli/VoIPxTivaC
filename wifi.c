/*
 * wifi.c -- Wifi module
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resource */
#include "common.h"
#include "dmaBuffer.h"
#include "dma.h"
#include "abm.h"
#include "wifi.h"

/* RX definitions */
#define WIFI_RX_BLOCK_SIZE      32
#define WIFI_RX_BLOCK_NUM_OF    5

/* TX definitions */
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
wifiXferBlock_t wifiTxPktBlock[WIFI_PKT_NUM_OF];
uint8_t wifiTxPktPayload0[WIFI_PKT_SIZE];
uint8_t wifiTxPktPayload1[WIFI_PKT_SIZE];
uint8_t wifiTxPktPayload2[WIFI_PKT_SIZE];
uint8_t wifiTxPktPayload3[WIFI_PKT_SIZE];

/* TX test packet */
#if WIFI_TEST_CANNED_TX_PKT
wifiXferBlock_t wifiTxTestPktBlock[1];
uint8_t wifiTxTestPktPayload0[WIFI_PKT_SIZE];
#endif 

/* AT command */
wifiXferBlock_t wifiAtBlock[3];
uint8_t wifiAtCmd0[] = "AT+CIPMUX=1\r\n";
uint8_t wifiAtCmd1[] = "AT+CIPSTART=4,\"UDP\",\"192.168.4.2\",56853\r\n";
uint8_t wifiAtCmd2[] = "AT+CIPSEND=4,172\r\n";

/** 
 * Request UART RX DMA transfer
 * param: none
 * return: none
**/
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
        /* error */
        abmAbort();
        return;
    }
    dmaBufferPut(
        &wifiCb.wifiRxBuffer,
        DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_2);
    len = WIFI_RX_BLOCK_SIZE;

    dmaChRequest(
        WIFI_DMA_RX_CH,
        (uint32_t *) &UART4_DR_R,
        (uint32_t *) addr,
        len);
}

/** 
 * Request UART TX DMA transfer
 * param: none
 * return: none
**/
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
        (uint32_t *) addr, 
        (uint32_t *) &UART4_DR_R, 
        len);
}

/** 
 * Initialize DMA
 * param: none
 * return: none
**/
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

/** 
 * Initialize TX packet blocks
 * param: none
 * return: none
**/
static void wifiTxPktInit(void)
{
    /* initialize packet buffer */
    dmaBufferInit(
        &wifiCb.wifiTxPktBuffer,
        (void *) &wifiTxPktBlock[0],
        sizeof(wifiTxPktBlock),
        sizeof(wifiTxPktBlock[0]));
    
    /* initialize element in packet buffer */
    wifiTxPktBlock[0].wifiPktP = &wifiTxPktPayload0[0];
    wifiTxPktBlock[0].wifiPktSize = sizeof(wifiTxPktPayload0);

    wifiTxPktBlock[1].wifiPktP = &wifiTxPktPayload1[0];
    wifiTxPktBlock[1].wifiPktSize = sizeof(wifiTxPktPayload1);

    wifiTxPktBlock[2].wifiPktP = &wifiTxPktPayload2[0];
    wifiTxPktBlock[2].wifiPktSize = sizeof(wifiTxPktPayload2);

    wifiTxPktBlock[3].wifiPktP = &wifiTxPktPayload3[0];
    wifiTxPktBlock[3].wifiPktSize = sizeof(wifiTxPktPayload3);

    #if WIFI_TEST_CANNED_TX_PKT
    int i;
    for (i = 0; i < sizeof(wifiTxTestPktPayload0); i++)
    {
        wifiTxTestPktPayload0[i] = '0' + (i %10);
    }
    wifiTxTestPktBlock[0].wifiPktP = &wifiTxTestPktPayload0[0];
    wifiTxTestPktBlock[0].wifiPktSize = sizeof(wifiTxTestPktPayload0);
    #endif
}

/** 
 * Initialize AT command blocks
 * param: none
 * return: none
**/
static void wifiAtInit(void)
{
    /* AT command 0 */
    wifiAtBlock[0].wifiPktP = &wifiAtCmd0[0];
    wifiAtBlock[0].wifiPktSize = 13;

    /* AT command 1 */
    wifiAtBlock[1].wifiPktP = &wifiAtCmd1[0];
    wifiAtBlock[1].wifiPktSize = 41;

    /* AT command 2 */
    wifiAtBlock[2].wifiPktP = &wifiAtCmd2[0];
    wifiAtBlock[2].wifiPktSize = 18;
}

/** 
 * Initialize UART
 * param: none
 * return: none
**/
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

/** 
 * Background task
 * param: none
 * return: none
**/
int wifiTxBackgroundTask2(int c)
{
    wifiXferBlock_t *pktP = (wifiXferBlock_t *) 0;

    if ((UDMA_ENASET_R & (1 << WIFI_DMA_TX_CH)) != 0)
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }

    /* determine c */
    if ((c == 0) || (c == 1))
    {
        /* AT command 0 and 1 */
        pktP = &wifiAtBlock[c];
    }
    else if (wifiCb.wifiTxPktBuffer.numOfAvail > 0)
    {
        if (c == 2)
        {
            pktP = &wifiAtBlock[2];
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
            pktP = &wifiTxTestPktBlock[0];
            #endif
        }
    }
    else
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }

    if (pktP == 0)
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }

    /* perform DMA transfer */
    wifiDmaTxRequest(
        pktP->wifiPktP, 
        pktP->wifiPktSize);

    wifiDmaRxRequest();

    return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Initialize module 
 * param: none
 * return: (int) -> execution status
**/
int wifiInit(void)
{
    /* initialize control block */
    dmaBufferInit(
        &wifiCb.wifiRxBuffer, 
        (void *) &wifiRxArray[0], 
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

    return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Read data from Wifi module
 * param: dataP -> output data reference
 * param: dataLen -> output data size in bytes
* return: (int) -> execution status
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

    return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * Send packet (get packet reference)
 * param: none
 * return: (wifiXferBlock_t*) -> packet reference
**/
wifiXferBlock_t* wifiPktSend1(void)
{
    void *ret;

    ret = dmaBufferPut(
            &wifiCb.wifiTxPktBuffer,
            DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1);

    if (ret == 0)
    {
        dmaBufferGet(
            &wifiCb.wifiTxPktBuffer,
            DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);

        dmaBufferGet(
            &wifiCb.wifiTxPktBuffer,
            DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);

        ret = dmaBufferPut(
            &wifiCb.wifiTxPktBuffer,
            DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1);
    }

    if (ret == 0)
    {
        abmAbort();
    }

    return (wifiXferBlock_t *) ret;
}

/** 
 * Send packet (actual send the written packet, 
 *             returned by wifiPktSend1)
 * param: none
 * return: (int) -> execution status
**/
int wifiPktSend2(void)
{
    dmaBufferPut(
        &wifiCb.wifiTxPktBuffer, 
        DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_2);

    return COMMON_RETURN_STATUS_SUCCESS;
}
