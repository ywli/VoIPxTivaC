//*****************************************************************************
//
// wifi.h - WiFi module
//
//
//*****************************************************************************

#ifndef __WIFI_H__
#define __WIFI_H__
#include <stdint.h>

#define WIFI_STATUS_SUCCESS 1
#define WIFI_STATUS_FAILURE 0

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
