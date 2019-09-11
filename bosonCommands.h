#ifndef __BOSON_COMMANDS_H__
#define __BOSON_COMMANDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "nvmedia_core.h"

typedef enum {
    WHITE_HOT = 0,
    BLACK_HOT
} BosonColor;

typedef enum {
    MANUAL_FFC = 0,
    AUTO_FFC
} FFCMode;

typedef enum {
    PACKING_DEFAULT = 0,
    PACKING_Y,
    PACKING_8_BIT,
    PACKING_END
} TelemetryPacking;

void
PackingToString(uint32_t val, char *outStr);

void
FFCModeToString(uint32_t val, char *outStr);

void
ColorToString(uint32_t val, char *outStr);

NvMediaStatus
TriggerShutter(uint32_t i2cDevice, uint32_t sensorAddress);

NvMediaStatus
GetSerialNumber(uint32_t i2cDevice, uint32_t sensorAddress, uint32_t *sn);

NvMediaStatus
SetColors(uint32_t i2cDevice, uint32_t sensorAddress, BosonColor color);

NvMediaStatus
SetFFCMode(uint32_t i2cDevice, uint32_t sensorAddress, FFCMode mode);

NvMediaStatus
GetColorMode(uint32_t i2cDevice, uint32_t sensorAddress, BosonColor *color);

NvMediaStatus
GetFFCMode(uint32_t i2cDevice, uint32_t sensorAddress, FFCMode *mode);

NvMediaStatus
GetTelemetryPacking(uint32_t i2cDevice, uint32_t sensorAddress,
    TelemetryPacking *packing);

#endif
