// TI library
#include "tm4c123gh6pm.h"

#include "cbuff_dma.h"
#include "sys.h"

#include <stdint.h>

#if 0
#define SPK_BUFFER_NUM_OF_SP 1600
#define SPK_SAMPLE_SIZE      2
#define SPK_BUFFER_SIZE      (SPK_BUFFER_NUM_OF_SP * SPK_SAMPLE_SIZE)
#define SPK_BLOCK_SIZE       SPK_BUFFER_SIZE/4
#define SPK_DMA_CH           11
#define SPK_ARCH             1 //1- SPI slave + PWM, 2- SPI master

struct spkControlBlock
{
	uint16_t spkSample[SPK_BUFFER_NUM_OF_SP];
	cbuff_dma_t spkBuffer;
};

struct spkControlBlock spkCb;

/* 8ksps, 1011Hz, 8sp one cycle, max amp.: 32k */
uint16_t spkTestData[] = 
{
0x3e80, 0x6ab2, 0x7d00, 0x6ab2, 0x3e80, 0x124e, 0x0000, 0x124e
};

void spkDma(void)
{
    uint8_t dmaChId;
	uint16_t *sampleP;
	uint32_t numOfSample;

	/* only transfer when buffer level reach to some defined level revise */
	/**/
	
	/* get data from buffer */
	// if (cbuff_dma_dequeue_driver1(
	// 	  	&spkCb.spkBuffer,
	// 	  	(unsigned char **) &sampleP,
	// 	  	&numOfSample) == 0)
	// {
	// 	return;
	// }
	sampleP = &spkTestData[0];
	numOfSample = sizeof(spkTestData) / sizeof(spkTestData[0]);

	/* transfer block using DMA */
    dmaChId = SPK_DMA_CH;

    /* still running */
    if (UDMA_ENASET_R & (1 << dmaChId))
    {
        return;
    }

    /* clear DMA status bit */
    UDMA_CHIS_R |= (1 << dmaChId);

    /* DMA channel control description */
    dmaTable[dmaChId].srcAddr = (uint32_t) (sampleP + numOfSample - 1);
	dmaTable[dmaChId].dstAddr = (uint32_t) &SSI0_DR_R;
	dmaTable[dmaChId].chCtl = UDMA_CHCTL_DSTINC_NONE |
							  UDMA_CHCTL_DSTSIZE_16  |
							  UDMA_CHCTL_SRCINC_16   |
							  UDMA_CHCTL_SRCSIZE_16  |
							  UDMA_CHCTL_ARBSIZE_4   |//revise
							  (((numOfSample - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M)|
                              UDMA_CHCTL_XFERMODE_BASIC;

    /* re-enable DMA */
    UDMA_ENASET_R |= (1 << dmaChId);

	return;
}
int test = 0;

void spkISR(void)
{
	test++;
	/* confirm block DMA transfer */
	//cbuff_dma_dequeue_driver2(&spkCb.spkBuffer);

	/* prepare next DMA transfer */
	spkDma();

	return;
}

int spkInit(void)
{
	cbuff_dma_init(
		&spkCb.spkBuffer, 
		(unsigned char*)&spkCb.spkSample[0],
		SPK_BUFFER_SIZE,
		SPK_BLOCK_SIZE);
	
	/* 
	 * PWM
	 * 8k frame clock 
	 * 32* 8k bit clock 
	 */
	#if (SPK_ARCH == 1)
	/* 5- PWM clock is system clock */
	SYSCTL_RCC_R &= ~SYSCTL_RCC_USEPWMDIV;

	/* 6- configue PWM generator */
	PWM0_2_CTL_R = 0;
	PWM0_1_CTL_R = 0;
	PWM0_2_GENB_R = PWM_2_GENB_ACTLOAD_ZERO|
					PWM_2_GENB_ACTCMPAD_ONE;
	PWM0_1_GENA_R = PWM_1_GENA_ACTLOAD_ZERO|
					PWM_1_GENA_ACTCMPAD_ONE;

	/* 7- set period */
	PWM0_2_LOAD_R = 312; //N1 = round(80M/32/8k)
	PWM0_1_LOAD_R = 9984;//N1*32

	/* 8- pulse width */
	PWM0_2_CMPB_R = 156;//N1/2
	PWM0_1_CMPA_R = 4992;//N1*16

	/* 9- start PWM timer */
	PWM0_2_CTL_R |= PWM_2_CTL_ENABLE | PWM_2_CTL_DEBUG;
	PWM0_1_CTL_R |= PWM_1_CTL_ENABLE | PWM_1_CTL_DEBUG;

	/* 10- enable PWM output */
	PWM0_ENABLE_R |= PWM_ENABLE_PWM5EN|
	                 PWM_ENABLE_PWM2EN;

	#endif

	/* 
	 * SPI 
	 */
	/* 1- enable ssi0 module */
	SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R0;
	while ((SYSCTL_RCGCSSI_R & SYSCTL_RCGCSSI_R0) == 0)
	{};
	
	/* 2-, 3-, 4-, 5- omitted */

	/* 1- disable */
	SSI0_CR1_R = 0;
	while (SSI0_CR1_R != 0)
	{}
	
	#if (SPK_ARCH == 1)
	/* 2- set slave SSI */
	SSI0_CR1_R = SSI_CR1_MS;
	#else
	/* 2- set master SSI */
	SSI0_CR1_R = 0;
	#endif

	/* 3- configure SSI clock to system clock */
	SSI0_CC_R = 0;

	/* 4- configure clock prescale divisor */
	SSI0_CPSR_R = 4;//bit rate 80M/4/(77+1)

	/* 5- configuration */
	SSI0_CR0_R = ((77 << SSI_CR0_SCR_S) & SSI_CR0_SCR_M)|
				 SSI_CR0_FRF_MOTO|
				 SSI_CR0_DSS_16;

	/* 6- uDMA config */
	SSI0_DMACTL_R |= SSI_DMACTL_TXDMAE;

	/* enable interrupt, interrupt 7 SSI0*/
	NVIC_EN0_R |= (1<<7);
	SSI0_IM_R = SSI_IM_TXIM;

	/* 7- enable SSI0 */
	SSI0_CR1_R |= SSI_CR1_SSE;

    /* 
	 * set a DMA channel for transmit 
	 */
	uint8_t dmaChId;
	
	/* channel attribut */
	dmaChId = SPK_DMA_CH;

	/* configure channel assignment, CH11, enc 0 = SSI0TX */
	UDMA_CHMAP1_R &= ~UDMA_CHMAP1_CH11SEL_M;
	UDMA_CHMAP1_R |= ((0 << UDMA_CHMAP1_CH11SEL_S) & UDMA_CHMAP1_CH11SEL_M);

	/* 1- default channel priority level, omitted */

	/* 2- use primary control structure, omitted */

	/* 3- respond to both single and burst requests, omitted */
	
	/* 4- clear mask */
	UDMA_REQMASKCLR_R |= (1 << dmaChId);

	spkDma();

	return 0;
}

int spkWrite(
	uint16_t *sampleP,
	uint16_t numOfSample)
{
	// /* put data into DMA buffer */
	// if (cbuff_dma_enqueue_app(
	// 	&spkCb.spkBuffer,
	// 	(unsigned char *)sampleP,
	// 	(numOfSample * 2)) == 0)
	// {
	// 	return 0;
	// }

	/* call DMA */
	spkDma();

	return numOfSample;
}

#endif
