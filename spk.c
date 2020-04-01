/*
 * spk.c -- speaker module
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resources */
#include "common.h"
#include "sys.h"
#include "dma.h"
#include "abm.h"
#include "dmaBuffer.h"

#if 1
#define SPK_BUFFER_NUM_OF_SP 1600
#define SPK_SAMPLE_SIZE      2
#define SPK_BUFFER_SIZE      (SPK_BUFFER_NUM_OF_SP * SPK_SAMPLE_SIZE)
#define SPK_BLOCK_SIZE       SPK_BUFFER_SIZE/4

/*
 * DMA definition
 */
/* DMA channel index */
#define SPK_DMA_CH            11
#define SPK_DMA_ENC           0
#define SPK_DMA_MODE          DMA_MODE_BASIC
#define SPK_DMA_DIR           DMA_DIR_RAM_TO_IO
#define SPK_DMA_ELEMENT_SIZE  2

#define SPK_ARCH             1 //1- SPI slave + PWM, 2- SPI master

struct spkControlBlock
{
	dmaBuffer_t spkBuffer;
};

uint16_t spkSample[SPK_BUFFER_NUM_OF_SP];

struct spkControlBlock spkCb;

/* 8ksps, 1011Hz, 8sp one cycle, max amp.: 32k */
uint16_t spkTestData[] = 
{
0x3e80, 0x6ab2, 0x7d00, 0x6ab2, 0x3e80, 0x124e, 0x0000, 0x124e
};

void spkDma(void)
{
	uint16_t *sampleP;
	uint32_t numOfSample;

	/* only transfer when buffer level reach to some defined level revise */
	/**/
	
	/* get data from buffer */
	sampleP = (uint16_t *) dmaBufferGet(
					&spkCb.spkBuffer,
					DMA_BUFFER_GET_OPT_DRV_GET_UNIT_1);

	if (sampleP == 0)
	{
		return;
	}
	dmaBufferGet(
		&spkCb.spkBuffer,
		DMA_BUFFER_GET_OPT_DRV_GET_UNIT_2);

	sampleP = &spkTestData[0];
	numOfSample = sizeof(spkTestData) / sizeof(spkTestData[0]);

	/* transfer block using DMA */
	dmaChRequest(
		SPK_DMA_CH, 
		(void *) sampleP, 
		(void *) &SSI0_DR_R, 
		numOfSample);

	return;
}

void spkISR(void)
{
	/* confirm block DMA transfer */
	//tbd 

	/* prepare next DMA transfer */
	spkDma();

	return;
}

int spkInit(void)
{	
	dmaBufferInit(
		&spkCb.spkBuffer, 
		(void *) &spkSample[0],
		sizeof(spkSample),
		SPK_BLOCK_SIZE);
	/* 
	 * PWM
	 * 8k frame clock 
	 * 32* 8k bit clock 
	 */
	#if (SPK_ARCH == 1)

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

    /* set a DMA channel for transmit */
	dmaChInit(
		SPK_DMA_CH, 
		SPK_DMA_ENC,
		SPK_DMA_MODE,
		SPK_DMA_DIR,
		SPK_DMA_ELEMENT_SIZE);

	spkDma();

	return COMMON_RETURN_STATUS_SUCCESS;
}

int spkWrite(
	int16_t *sampleP,
	uint16_t numOfSample)
{
	int16_t *dstSampleP;

	/* put data into DMA buffer */
	dstSampleP = (int16_t *) dmaBufferPut(
		&spkCb.spkBuffer,
		DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1);
	
	/* if overrun, flush the old block */
	if (dstSampleP == 0)
	{
		/* overrun */
		dmaBufferGet(
		&spkCb.spkBuffer,
		DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);

		dmaBufferGet(
		&spkCb.spkBuffer,
		DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);

		dstSampleP = (int16_t *) dmaBufferPut(
			&spkCb.spkBuffer,
			DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1);
	}

	/* if still no space */
	if (dstSampleP == 0)
	{
		abmAbort();
	}

	dmaBufferPut(
		&spkCb.spkBuffer,
		DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_2);

	int i;
	for (i = 0; i < numOfSample; i++)
	{
		dstSampleP[i] = sampleP[i];
	}

	/* call DMA */
	spkDma();

	return COMMON_RETURN_STATUS_SUCCESS;
}

#endif
