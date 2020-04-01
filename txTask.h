//*****************************************************************************
//
// txTask.h - tx module
//
//
//*****************************************************************************

#ifndef __TX_H__
#define __TX_H__

/** 
 * Initialize TX task
 * param: none
 * return: none
**/
void txInit(void);

/** 
 * TX task
 * param: pvParameters-> task argument
 * return: none
**/
void txTask(void *pvParameters);

#endif // __TX_H__
