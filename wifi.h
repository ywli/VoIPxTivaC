//*****************************************************************************
//
// wifi.h - WiFi module
//
//
//*****************************************************************************

#ifndef __WIFI_H__
#define __WIFI_H__
#include <stdint.h>


int wifiInit(void);

int wifiRead(
    uint8_t *dataP,
    uint16_t dataLen);

int wifiWrite(
    uint8_t *dataP,
    uint16_t dataLen);

int wifiConnect(void);

int wifiPktSend(
    unsigned char *dataP,
    int dataLen);
    
void wifiBackgroundTask(void);
#endif // __WIFI_H__
