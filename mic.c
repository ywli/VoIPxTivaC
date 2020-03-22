// TI library
#include "tm4c123gh6pm.h"
#include "sys.h"
#include <stdint.h>


#define MIC_BUFFER_NUM_OF_SP 160
#define MIC_BUFFER_SIZE      640

struct micControlBlock
{
	uint16_t micBuffer[MIC_BUFFER_SIZE];
	uint16_t micBufferWrIdx;
	uint16_t micBufferRdIdx;
	uint16_t micBufferAvailable;
	uint8_t micDmaPhase;
};

static struct micControlBlock micCb;

int micInit(void)
{
	uint8_t dmaChId;
	uint16_t i;

	for (i = 0; i < MIC_BUFFER_SIZE; i++)
	{
		micCb.micBuffer[i] = 0xffff;
	}
	micCb.micBufferRdIdx = 0;
	micCb.micBufferWrIdx = 0;
	micCb.micBufferAvailable = 0;
	micCb.micDmaPhase = 0;

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
	TIMER0_CTL_R = TIMER_CTL_TAOTE | TIMER_CTL_TAEN;

	/* 8 omitted */

	/* 
	 * set a DMA channel for ping pong receive 
	 */

	/* channel attribut */
	dmaChId = 14;

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
	/* primary */
	dmaTable[dmaChId].dstAddr = (uint32_t) &micCb.micBuffer[MIC_BUFFER_NUM_OF_SP - 1];
	dmaTable[dmaChId].srcAddr = (uint32_t) &ADC0_SSFIFO0_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_16  |
							  UDMA_CHCTL_DSTSIZE_16 |
							  UDMA_CHCTL_SRCINC_NONE|
							  UDMA_CHCTL_SRCSIZE_16 |
							  UDMA_CHCTL_ARBSIZE_1  |//revise
							  (((MIC_BUFFER_NUM_OF_SP - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
							  UDMA_CHCTL_NXTUSEBURST | //revise
							  UDMA_CHCTL_XFERMODE_PINGPONG;

	/* alternative */
	dmaTable[dmaChId + 32].dstAddr = (uint32_t) &micCb.micBuffer[2*MIC_BUFFER_NUM_OF_SP - 1];
	dmaTable[dmaChId + 32].srcAddr = dmaTable[dmaChId].srcAddr;
	dmaTable[dmaChId + 32].chCtl = dmaTable[dmaChId].chCtl;

	/* configure peripheral interrupt revise */

	/* enable uDMA channel */
	UDMA_ENASET_R |= (1 << dmaChId);

	return 0;
}

void micISR(void)
{
	uint8_t dmaChId;
	uint32_t idx;

	/* index increment */
	micCb.micBufferWrIdx += MIC_BUFFER_NUM_OF_SP;
	micCb.micBufferWrIdx %= MIC_BUFFER_SIZE;
	if (micCb.micBufferAvailable < MIC_BUFFER_SIZE)
	{
		micCb.micBufferAvailable += MIC_BUFFER_NUM_OF_SP;
	}
	if (micCb.micDmaPhase == 0)
	{
		micCb.micDmaPhase = 1;
		dmaChId = 14;
	}
	else
	{
		micCb.micDmaPhase = 0;
		dmaChId = 14 + 32;
	}
	
	idx = (micCb.micBufferWrIdx + MIC_BUFFER_NUM_OF_SP)%MIC_BUFFER_SIZE;
	idx += MIC_BUFFER_NUM_OF_SP - 1;

	/* reload DMA channel control structure */
	dmaTable[dmaChId].dstAddr = (uint32_t) &micCb.micBuffer[idx];
	dmaTable[dmaChId].srcAddr = (uint32_t) &ADC0_SSFIFO0_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_16  |
							  UDMA_CHCTL_DSTSIZE_16 |
							  UDMA_CHCTL_SRCINC_NONE|
							  UDMA_CHCTL_SRCSIZE_16 |
							  UDMA_CHCTL_ARBSIZE_1  |//revise
							  (((MIC_BUFFER_NUM_OF_SP - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
							  UDMA_CHCTL_NXTUSEBURST | //revise
							  UDMA_CHCTL_XFERMODE_PINGPONG;

	/* enable uDMA channel */
	UDMA_ENASET_R |= (1 << dmaChId);
	return;
}

uint16_t* micReadBlock(void)
{
	uint16_t *res;

	res = 0;
	if (micCb.micBufferAvailable >= MIC_BUFFER_NUM_OF_SP)
	{
		res = &micCb.micBuffer[micCb.micBufferRdIdx];
		micCb.micBufferRdIdx = (micCb.micBufferRdIdx + MIC_BUFFER_NUM_OF_SP)%MIC_BUFFER_SIZE;
		micCb.micBufferAvailable -= MIC_BUFFER_NUM_OF_SP;
	}

	return res;
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

