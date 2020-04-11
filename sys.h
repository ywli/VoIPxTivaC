//*****************************************************************************
//
// mic.h - Microphone module
//
//
//*****************************************************************************
#include <stdint.h>

#ifndef __SYS_H__
#define __SYS_H__

/* system clock frequency in Hz */
#define SYS_SYSTEM_CLOCK_HZ  80000000

/** 
 * Initialize system resources
 * param: none
 * return: none
**/
int sysInit(void);


#endif // __SYS_H__
