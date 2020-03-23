// TI library
#include "sys.h"
#include "cbuff_dma.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>


#define MIC_BLOCK_MS         20
#define MIC_SAMPLE_RATE_HZ   8000
#define MIC_BLOCK_NUM_OF_SP  (MIC_SAMPLE_RATE_HZ * MIC_BLOCK_MS / 1000)
#define MIC_BLOCK_SIZE       (MIC_BLOCK_NUM_OF_SP * sizeof(uint16_t))
#define MIC_BUFFER_NUM_OF_SP (MIC_BLOCK_NUM_OF_SP * 4)
#define MIC_BUFFER_SIZE      (MIC_BUFFER_NUM_OF_SP * sizeof(uint16_t))
#define MIC_DMA_CH           14
struct micControlBlock
{
	uint16_t micBufferArray[MIC_BUFFER_NUM_OF_SP];
	cbuff_dma_t micBuffer;
};

struct micControlBlock micCb;

static void micDma(void)
{
    static uint8_t phase = 0;
    uint8_t dmaChId;
    uint16_t *addr;
    int32_t len;

    /* prepare space in DMA buffer */
    if (cbuff_dma_enqueue_driver1(
        &micCb.micBuffer, 
        (unsigned char*)&addr, 
        &len) == 0)
	{
		return;
	}

    /* phase process */
    if (phase == 0)
    {
        phase = 1;
        dmaChId = MIC_DMA_CH;
    }
    else
    {
        phase = 0;
        dmaChId = MIC_DMA_CH + 32;
    }

	len = len/2;

    /* DMA channel control description */
	dmaTable[dmaChId].dstAddr = (uint32_t) (addr + len - 1);
	dmaTable[dmaChId].srcAddr = (uint32_t) &ADC0_SSFIFO0_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_16  |
							  UDMA_CHCTL_DSTSIZE_16 |
							  UDMA_CHCTL_SRCINC_NONE|
							  UDMA_CHCTL_SRCSIZE_16 |
							  UDMA_CHCTL_ARBSIZE_1  |//revise
							  (((len - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
							  UDMA_CHCTL_NXTUSEBURST | //revise
							  UDMA_CHCTL_XFERMODE_PINGPONG;
}

int micInit(void)
{
	const uint8_t dmaChId = MIC_DMA_CH;

	cbuff_dma_init(
		&micCb.micBuffer,
		(unsigned char*) &micCb.micBufferArray[0],
		MIC_BUFFER_SIZE,
		MIC_BLOCK_SIZE);

	/* assuming Sec. 13.4.1 module init and 13.4.2 sample sequence config are complete*/

	/* 
	 * ADC module init 
	 */

	/* 1- enable clock to ADC module 0 omitted */

	/* 2-, 3-, 4-, 5- omitted */

	/* step 6 omitted*/

	/*
	 * sample sequencer config 
	 */

	/* 1- disable all sample sequencer, p. 821 */
	ADC0_ACTSS_R = 0x00000000;

	/* 2- Timer for the sample sequencer trigger event, p. 837 */
	ADC0_EMUX_R |= ADC_EMUX_EM0_TIMER;

	/* 3- omitted, p.839 */

	/* 4- input source AIN0, p.851*/
	ADC0_SSMUX0_R = 0x00000000;

	/* 5- configure sample control bits, p.853 */
	ADC0_SSCTL0_R |= ADC_SSCTL0_END0|
					 ADC_SSCTL0_IE0;

	/* 6- SS0 interrupt is sent to intterrupt controller, p.825 */
	ADC0_IM_R |= ADC_IM_MASK0;
	NVIC_EN0_R |= (1<<14);

	/* 7- enable the sample sequencer0, p. 821  */
	ADC0_ACTSS_R |= ADC_ACTSS_ASEN0;

	/* process interrupts revise */

	/* 
	 * configure trigger source timer 
	 */

	/* 1 disable timer, p.737 */
	TIMER0_CTL_R = 0x00000000;

	/* 2 config register 16-bit timer, p.727 */
	TIMER0_CFG_R = TIMER_CFG_16_BIT;

	/* 3 configure timer mode, p.729 */
	TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;//revise

	/* 5 set start value */
	TIMER0_TAILR_R = 10000;//80 MHz / 8 kHz

	/* 6 set interrupt */
	//TIMER0_IMR_R = 0x00000001;//revise

	/* 7 enable timer, p.737 */
	//TIMER0_CTL_R = TIMER_CTL_TAOTE | TIMER_CTL_TAEN;

	/* 8 omitted */

	/* 
	 * set a DMA channel for ping pong receive 
	 */

	/* channel attribut */
	
	/* configure channel assignment, CH14, enc 0 = ADC0 SS0 */
	UDMA_CHMAP1_R &= ~UDMA_CHMAP1_CH14SEL_M;
	UDMA_CHMAP1_R |= ((0 << UDMA_CHMAP1_CH14SEL_S) & UDMA_CHMAP1_CH14SEL_M);

	/* 1- default channel priority level */
	UDMA_PRIOSET_R = 0x00000000;

	/* 2- use primary control structure */
	UDMA_ALTSET_R = 0x00000000;

	/* 3- respond to both single and burst requests */
	UDMA_USEBURSTCLR_R = 0x00000000;
	
	/* 4- clear mask */
	UDMA_REQMASKCLR_R |= (1 << dmaChId);

	/* channel control structure */
	micDma();
	micDma();

	/* configure peripheral interrupt */

	/* enable uDMA channel */
	UDMA_ENASET_R |= (1 << dmaChId);

	TIMER0_CTL_R = TIMER_CTL_TAOTE | TIMER_CTL_TAEN;

	return 0;
}

void micISR(void)
{
	const uint8_t dmaChId = MIC_DMA_CH;

	/* confirm one block received */
	cbuff_dma_enqueue_driver2(&micCb.micBuffer);

	/* reload next block */
	micDma();

	/* enable uDMA channel */
	UDMA_ENASET_R |= (1 << dmaChId);

	return;
}

int micReadBlock(uint16_t *blockP)
{
	return cbuff_dma_dequeue_app(
				&micCb.micBuffer,
				(unsigned char *) blockP,
				MIC_BLOCK_SIZE);
}

int micStart(void)
{
	TIMER0_CTL_R |= (TIMER_CTL_TAOTE | TIMER_CTL_TAEN);
	return 0;
}

int micStop(void)
{
	TIMER0_CTL_R &= ~(TIMER_CTL_TAOTE | TIMER_CTL_TAEN);
	return 0;
}

