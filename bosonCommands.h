#ifndef __BOSON_COMMANDS_H__
#define __BOSON_COMMANDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "nvmedia_core.h"

typedef enum {
    PACKING_DEFAULT = 0,
    PACKING_Y,
    PACKING_8_BIT,
    PACKING_END
} TelemetryPacking;

void
PackingToString(uint32_t val, char *outStr);

void
VideoTypeToString(uint32_t val, char *outStr);

NvMediaStatus
RunCommandWithInt32Response(uint32_t i2cDevice, uint32_t sensorAddress, 
    uint16_t *cmdBody, uint32_t *resp);

NvMediaStatus
RunCommandWithStringResponse(uint32_t i2cDevice, uint32_t sensorAddress, 
    uint16_t *cmdBody, char *resp, uint32_t length);

NvMediaStatus
RunVoidCommand(uint32_t i2cDevice, uint32_t sensorAddress, uint16_t *cmdBody,
    uint32_t *param);

NvMediaStatus
TriggerShutter(uint32_t i2cDevice, uint32_t sensorAddress);

NvMediaStatus
ToggleHeater(uint32_t i2cDevice, uint32_t sensorAddress);

NvMediaStatus
GetSerialNumber(uint32_t i2cDevice, uint32_t sensorAddress, uint32_t *sn);

NvMediaStatus
GetTelemetryPacking(uint32_t i2cDevice, uint32_t sensorAddress,
    TelemetryPacking *packing);

NvMediaStatus
GetPartNumber(uint32_t i2cDevice, uint32_t sensorAddress, char *pn);

NvMediaStatus
GetIntValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr,
    uint32_t *result);

NvMediaStatus
GetStringValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr,
    char *result);

NvMediaStatus
SetIntValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr, 
    char *arg);

NvMediaStatus
GetFPS(uint32_t i2cDevice, uint32_t sensorAddress, uint32_t *fps);

#endif
