/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resource */
#include "sys.h"
#include "dma.h"
#include "dmaBuffer.h"
#include "mic.h"
#include "abm.h"

/* definition of control block */
typedef struct 
{
	dmaBuffer_t micBuffer;
}micControlBlock_t;

/* audio buffer */
micDataBlock_t micBuffer[MIC_BLOCK_NUM_OF];

/* control block */
micControlBlock_t micCb;

/** 
 * Module ISR callback
 * param: none
 * return: none
**/
void micISR(void)
{
	/* confirm one block received */
	dmaBufferPut(
		&micCb.micBuffer, 
		DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_2);

	/* reload next block */
	micDmaRequest();

	return;
}

/** 
 * Perform DMA transfer
 * param: none
 * return: none
**/
static void micDmaRequest(void)
{   
	micDataBlock_t *blockP;

    /* prepare space in DMA buffer */
	blockP = (micDataBlock_t *) dmaBufferPut(
									&micCb.micBuffer, 
									DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_1);
	/* no space for data */
	if (blockP == 0)
	{
		abmAbort();
	}

	dmaChRequest(
		MIC_DMA_CH, 
		(uint32_t *) &ADC0_SSFIFO0_R, 
		(uint32_t *) &blockP->micDataBlock[0], //sample reference
		MIC_BLOCK_NUM_OF_SP);                  //sample number
}

/** 
 * Initialize DMA
 * param: none
 * return: none
**/
void micDmaInit(void)
{
	dmaChInit(
		MIC_DMA_CH, 
		MIC_DMA_ENC, 
		MIC_DMA_MODE,
		MIC_DMA_DIR, 
		MIC_DMA_ELEMENT_SIZE);

	/* channel control structure */
	micDmaRequest();
	micDmaRequest();
}

/** 
 * Initialize ADC
 * param: none
 * return: none
**/
void micAdcInit(void)
{
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
	return;
}

/** 
 * Initialize Timer
 * param: none
 * return: none
**/
void micTimerInit(void)
{
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
	return;
}

/** 
 * Initialize module
 * param: none
 * return: (int) -> MIC_STATUS_SUCCESS, MIC_STATUS_FAILURE
**/
int micInit(void)
{
	/* initialize buffer */
	dmaBufferInit(
		&micCb.micBuffer, 
		(void*) &micBuffer[0], 
		sizeof(micBuffer), 
		sizeof(micBuffer[0]));
	
	/* initialize ADC */
	micAdcInit();

	/* initialize Timer */
	micTimerInit();

	/* initialize DMA */
	micDmaInit();

	/* automatic start */
	//micStart();

	return MIC_STATUS_SUCCESS;
}


/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Bu -Hp -o 2 -a 2.5000000000e-03 0.0000000000e+00 -l */
int micDcBlockFilter(
	int16_t input[], 
	int16_t output[], 
	int num)
#if MIC_DC_BLOCK_FILTER
{
	int i;
	#define NZEROS 2
	#define NPOLES 2
	#define GAIN   1.011169123e+00

	static int32_t xv[NZEROS+1], yv[NPOLES+1];

	for (i = 0; i < num; i++)
	{ xv[0] = xv[1]; xv[1] = xv[2]; 
		xv[2] = input[i];
		yv[0] = yv[1]; yv[1] = yv[2]; 
		yv[2] =   (1000*(xv[0] + xv[2]) - 2000 * xv[1]
					+ ( -978 * yv[0]) + (  1978 * yv[1]))/1000;
		output[i] = yv[2];
	}

	return MIC_STATUS_SUCCESS;
}
#else
{
	int i;
	for (i = 0; i < num; i++)
	{	
		output[i] = ((input[i] - 2048));
	}
	return MIC_STATUS_SUCCESS;
}
#endif

#if 0
/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Bu -Hp -o 2 -a 2.5000000000e-03 0.0000000000e+00 -l */

#define NZEROS 2
#define NPOLES 2
#define GAIN   1.011169123e+00

static float xv[NZEROS+1], yv[NPOLES+1];

static void filterloop()
  { for (;;)
      { xv[0] = xv[1]; xv[1] = xv[2]; 
        xv[2] = `next input value' / GAIN;
        yv[0] = yv[1]; yv[1] = yv[2]; 
        yv[2] =   (xv[0] + xv[2]) - 2 * xv[1]
                     + ( -0.9780305085 * yv[0]) + (  1.9777864838 * yv[1]);
        `next output value' = yv[2];
      }
  }
#endif

/** 
 * Get one audio block
 * param: none
 * return: (micDataBlock_t*) -> 
 *       retrieved audio block reference on valid address,
 *       0 on block not available
**/
micDataBlock_t* micBlockGet(void)
{
	void *ret;
	micDataBlock_t *blockP;

	/* get block reference */
	blockP = (micDataBlock_t *) dmaBufferGet(
									&micCb.micBuffer,
									DMA_BUFFER_GET_OPT_APP_GET_UNIT_1);
	/* no data */
	if (blockP == 0)
	{
		return 0;
	}

	dmaBufferGet(
		&micCb.micBuffer,
		DMA_BUFFER_GET_OPT_APP_GET_UNIT_2);
	
	#if(MIC_TEST_1011_TONE == 1)
	int i;
	/* 8ksps, 1011Hz, 8sp one cycle, max amp.: 32k */
	static const uint16_t micTestData[] = 
	{
		0x3e80, 0x6ab2, 0x7d00, 0x6ab2, 0x3e80, 0x124e, 0x0000, 0x124e
	};
	for (i = 0; i<160; i++)
	{
		blockP->micDataBlock[i] = micTestData[(i * 1 ) % 8];//1k tone
	}
	#else
	micDcBlockFilter(
        &blockP->micDataBlock[0],
        &blockP->micDataBlock[0],
        MIC_BLOCK_NUM_OF_SP);
	#endif

	return blockP;
}

/** 
 * Start audio capturing
 * param: none
 * return: (int) -> MIC_STATUS_SUCCESS, MIC_STATUS_FAILURE
**/
int micStart(void)
{
	TIMER0_CTL_R |= (TIMER_CTL_TAOTE | TIMER_CTL_TAEN);
	return MIC_STATUS_SUCCESS;
}

/** 
 * Stop audio capturing
 * param: none
 * return: (int) -> MIC_STATUS_SUCCESS, MIC_STATUS_FAILURE
**/
int micStop(void)
{
	TIMER0_CTL_R &= ~(TIMER_CTL_TAOTE | TIMER_CTL_TAEN);
	return MIC_STATUS_SUCCESS;
}
