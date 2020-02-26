/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
#ifndef __BOSON_INTERFACE_H__
#define __BOSON_INTERFACE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "nvmedia_core.h"

void
BuildCommand(uint16_t *cmdBody, uint32_t *value, uint16_t *outCmd);

NvMediaStatus
SendCommand(uint32_t i2cDevice, uint32_t sensorAddress, uint16_t *cmd);

NvMediaStatus
ReceiveData(uint32_t i2cDevice, uint32_t sensorAddress, uint8_t reg, 
    uint32_t *response);

NvMediaStatus
ReceiveStringData(uint32_t i2cDevice, uint32_t sensorAddress, uint8_t reg, 
    char *response, uint32_t length);


void
ResetI2CBuffer(uint32_t i2cDevice, uint32_t sensorAddress);

#endif