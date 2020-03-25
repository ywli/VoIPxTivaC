// TI library
#include "tm4c123gh6pm.h"
#include "sys.h"
#include "cbuff_dma.h"
#include <stdint.h>

#define WIFI_BUFFER_SIZE 120
#define WIFI_TRANSPORT_SIZE 60
#define WIFI_DMA_CH_RX 18
#define WIFI_DMA_CH_TX 19

struct wifiControlBlock
{
	uint8_t wifiTxArray[WIFI_BUFFER_SIZE];
    uint8_t wifiRxArray[WIFI_BUFFER_SIZE];
    cbuff_dma_t wifiTxBuffer;
    cbuff_dma_t wifiRxBuffer;
};

struct wifiControlBlock wifiCb;

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

static void wifiTxDma(void)
{
    uint8_t dmaChId;
    uint8_t *addr;
    int32_t len;

    dmaChId = WIFI_DMA_CH_TX;

    /* get data from buffer */
    if (cbuff_dma_dequeue_driver1(
        &wifiCb.wifiTxBuffer,
        (unsigned char **) &addr,
        &len) == 0)
    {
        return;
    }

    /* still running */
    if (UDMA_ENASET_R & (1 << dmaChId))
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

void wifiISR(void)
{
    uint32_t reg;

    reg = UART4_MIS_R;

    /* recv interrupt */
    if (reg & UART_MIS_RXMIS)
    {
        /* clear interrupt */
        //UART4_ICR_R |= UART_ICR_RXIC;

        /* confirm block received */
        cbuff_dma_enqueue_driver2(&wifiCb.wifiRxBuffer);

        /* prepare next block */
        wifiRxDma();
    }

    /* transmit interrupt */
    if (reg & UART_MIS_TXMIS)
    {
        /* clear interrupt */
        UART4_ICR_R |= UART_ICR_TXIC;
        
        /* confirm block sent */
        cbuff_dma_dequeue_driver2(&wifiCb.wifiTxBuffer);

        /* prepare next block */
        wifiTxDma();
    }
}

int wifiInit(void)
{
    uint8_t dmaChId;

    /* initialize control block */
    cbuff_dma_init(
        &wifiCb.wifiTxBuffer,
        &wifiCb.wifiTxArray[0],
        WIFI_BUFFER_SIZE,
        WIFI_TRANSPORT_SIZE);

    cbuff_dma_init(
        &wifiCb.wifiRxBuffer,
        &wifiCb.wifiRxArray[0],
        WIFI_BUFFER_SIZE,
        WIFI_TRANSPORT_SIZE);

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
    UART4_IM_R = UART_IM_TXIM|
                 UART_IM_RXIM;

    NVIC_EN1_R |= (1<<28);


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
        return -1;
    }

    /* activate DMA */
    wifiTxDma();

    return 0;
}
