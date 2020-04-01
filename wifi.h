//*****************************************************************************
//
// wifi.h - WiFi module
//
//
//*****************************************************************************

#ifndef __WIFI_H__
#define __WIFI_H__
#include <stdint.h>
#include "testMode.h"

/* Module test option */
#define WIFI_TEST_CANNED_TX_PKT   TEST_MODE_WIFI_CANNED_TX_PKT

/* 
 * DMA definitions 
 */
#define WIFI_DMA_RX_CH            18
#define WIFI_DMA_RX_ENC           2
#define WIFI_DMA_RX_MODE          DMA_MODE_BASIC
#define WIFI_DMA_RX_DIR           DMA_DIR_IO_TO_RAM
#define WIFI_DMA_RX_ELEMENT_SIZE  1

#define WIFI_DMA_TX_CH            19
#define WIFI_DMA_TX_ENC           2
#define WIFI_DMA_TX_MODE          DMA_MODE_BASIC
#define WIFI_DMA_TX_DIR           DMA_DIR_RAM_TO_IO
#define WIFI_DMA_TX_ELEMENT_SIZE  1

/* transfer block definition */
typedef struct{
    int wifiPktSize;
    uint8_t *wifiPktP;
}wifiXferBlock_t;

/** 
 * Initialize module 
 * param: none
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiInit(void);

/** 
 * Read data from Wifi module
 * param: dataP -> output data reference
 * param: dataLen -> output data size in bytes
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiRead(
    uint8_t *dataP,
    uint16_t dataLen);

/** 
 * Send packet (get packet reference)
 * param: none
 * return: (wifiXferBlock_t*) -> packet reference
**/
wifiXferBlock_t* wifiPktSend1(void);

/** 
 * Send packet (actual send the written packet, 
 *             returned by wifiPktSend1)
 * param: none
 * return: (int) -> WIFI_STATUS_SUCCESS, WIFI_STATUS_FAILURE
**/
int wifiPktSend2(void);

#endif // __WIFI_H__
