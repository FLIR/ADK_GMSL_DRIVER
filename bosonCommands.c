#include <string.h>

#include "os_common.h"
#include "log_utils.h"
#include "opencvConnector.h"
#include "bosonInterface.h"
#include "helpers.h"
#include "bosonCommands.h"

static uint16_t _cmd[64];

static void
_StringToCommand(uint16_t *cmdBody, char *cmdStr) {
    uint8_t tempCmd[4];
    uint32_t cmdNum = (uint32_t)strtoul(cmdStr, NULL, 16);
    LsbToMsbArr(tempCmd, cmdNum);
    for (size_t i = 0; i < 4; i++) {
        cmdBody[i] = tempCmd[i];
    }
}

void
PackingToString(uint32_t val, char *outStr) {
    if(val == PACKING_DEFAULT) {
        strcpy(outStr, "Default");
        return;
    }
    if(val == PACKING_Y) {
        strcpy(outStr, "Y");
        return;
    }
    if(val == PACKING_8_BIT) {
        strcpy(outStr, "8 Bit");
        return;
    }
}

void
VideoTypeToString(uint32_t val, char *outStr) {
    if(val == VIDEO_MONO16) {
        strcpy(outStr, "Mono 16");
        return;
    }
    if(val == VIDEO_MONO8) {
        strcpy(outStr, "Mono 8");
        return;
    }
    if(val == VIDEO_COLOR) {
        strcpy(outStr, "Color");
        return;
    }
    if(val == VIDEO_ANALOG) {
        strcpy(outStr, "Analog");
        return;
    }
}

void
FFCModeToString(uint32_t val, char *outStr) {
    if(val == MANUAL_FFC) {
        strcpy(outStr, "Manual");
        return;
    }
    if(val == AUTO_FFC) {
        strcpy(outStr, "Auto");
        return;
    }
}

void
ColorToString(uint32_t val, char *outStr) {
    if(val == WHITE_HOT) {
        strcpy(outStr, "White Hot");
        return;
    }
    if(val == BLACK_HOT) {
        strcpy(outStr, "Black Hot");
        return;
    }
}

static NvMediaStatus
_RunCommandWithResponseHelper(int32_t i2cDevice, uint32_t sensorAddress, 
    uint16_t *cmdBody)
{
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    BuildCommand(cmdBody, NULL, _cmd);
    ResetI2CBuffer(i2cDevice, sensorAddress);
    nvsleep(100);
    status = SendCommand(i2cDevice, sensorAddress, _cmd);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error sending command", __func__);
        return status;
    }

    nvsleep(1000);
    return status;
}

NvMediaStatus
RunCommandWithInt32Response(uint32_t i2cDevice, uint32_t sensorAddress, 
    uint16_t *cmdBody, uint32_t *resp)
{
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    status = _RunCommandWithResponseHelper(i2cDevice, sensorAddress, cmdBody);
    if(status != NVMEDIA_STATUS_OK) {
        return;
    }
    status = ReceiveData(i2cDevice, sensorAddress, 0, resp);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error receiving data", __func__);
    }

    return status;
}

NvMediaStatus
RunCommandWithStringResponse(uint32_t i2cDevice, uint32_t sensorAddress, 
    uint16_t *cmdBody, char *resp, uint32_t length)
{
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    status = _RunCommandWithResponseHelper(i2cDevice, sensorAddress, cmdBody);
    if(status != NVMEDIA_STATUS_OK) {
        return;
    }
    status = ReceiveStringData(i2cDevice, sensorAddress, 0, resp, length);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error receiving data", __func__);
    }

    return status;
}

NvMediaStatus
RunVoidCommand(uint32_t i2cDevice, uint32_t sensorAddress, uint16_t *cmdBody,
    uint32_t *param)
{
    BuildCommand(cmdBody, param, _cmd);
    return SendCommand(i2cDevice, sensorAddress, _cmd);
}

NvMediaStatus
TriggerShutter(uint32_t i2cDevice, uint32_t sensorAddress) {
    uint16_t cmdBody[4] = {0x00, 0x05, 0x00, 0x07};
    return RunVoidCommand(i2cDevice, sensorAddress, cmdBody, NULL);    
}

NvMediaStatus
ToggleHeater(uint32_t i2cDevice, uint32_t sensorAddress) {
    // uint16_t cmdBody[4] = 
}

NvMediaStatus
GetSerialNumber(uint32_t i2cDevice, uint32_t sensorAddress, uint32_t *sn) {
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x05, 0x00, 0x02};

    status = RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody, sn);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }

    return status;
}

NvMediaStatus
SetColors(uint32_t i2cDevice, uint32_t sensorAddress, BosonColor color) {
    uint32_t param = (uint32_t)color;
    uint16_t cmdBody[4] = {0x00, 0x0B, 0x00, 0x03};
    return RunVoidCommand(i2cDevice, sensorAddress, cmdBody, &param);
}

NvMediaStatus
SetFFCMode(uint32_t i2cDevice, uint32_t sensorAddress, FFCMode mode) {
    uint32_t param = (uint32_t)mode;
    uint16_t cmdBody[4] = {0x00, 0x05, 0x00, 0x12};
    return RunVoidCommand(i2cDevice, sensorAddress, cmdBody, &param);
}

NvMediaStatus
GetFFCMode(uint32_t i2cDevice, uint32_t sensorAddress, FFCMode *mode) {
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x05, 0x00, 0x13};
    uint32_t modeInt = (uint32_t)*mode;
    
    status = RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody, &modeInt);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }
    *mode = (FFCMode)modeInt;

    return status;
}

NvMediaStatus
GetColorMode(uint32_t i2cDevice, uint32_t sensorAddress, BosonColor *color) {
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x0B, 0x00, 0x04};
    uint32_t colorInt = (uint32_t)*color;

    status = RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody, &colorInt);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }
    *color = (BosonColor)colorInt;

    return status;
}

NvMediaStatus
GetTelemetryPacking(uint32_t i2cDevice, uint32_t sensorAddress,
    TelemetryPacking *packing)
{
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x04, 0x00, 0x06};
    uint32_t packingInt = (uint32_t)*packing;

    status = RunCommandWithInt32Response(i2cDevice, sensorAddress, 
        cmdBody, &packingInt);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }
    *packing = (TelemetryPacking)packingInt;

    return status;
}

NvMediaStatus
GetPartNumber(uint32_t i2cDevice, uint32_t sensorAddress, char *pn) {
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x05, 0x00, 0x3F};

    status = RunCommandWithStringResponse(i2cDevice, sensorAddress, 
        cmdBody, pn, 32);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }

    return status;
}

NvMediaStatus
GetVideoType(uint32_t i2cDevice, uint32_t sensorAddress, VideoType *vidType) {
    NvMediaStatus status;
    uint16_t cmdBody[4] = {0x00, 0x06, 0x00, 0x10};
    uint32_t vidInt = (uint32_t)*vidType;

    status = RunCommandWithInt32Response(i2cDevice, sensorAddress, 
        cmdBody, &vidInt);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error running response command", __func__);
    }
    *vidType = (VideoType)vidInt;

    return status;
}

NvMediaStatus
SetVideoType(uint32_t i2cDevice, uint32_t sensorAddress, VideoType vidType) {
    uint32_t param = (uint32_t)vidType;
    uint16_t cmdBody[4] = {0x00, 0x06, 0x00, 0x0F};
    BuildCommand(cmdBody, &param, _cmd);
    return SendCommand(i2cDevice, sensorAddress, _cmd);
}

NvMediaStatus
GetIntValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr,
    uint32_t *result)
{
    uint16_t cmdBody[4];
    _StringToCommand(cmdBody, cmdStr);
    
    return RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody,
        result);
}

NvMediaStatus
GetStringValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr,
    char *result)
{
    NvMediaStatus status;
    uint16_t cmdBody[4];
    _StringToCommand(cmdBody, cmdStr);
    
    status = RunCommandWithStringResponse(i2cDevice, sensorAddress, cmdBody, 
        result, 32);
    return status;
}

NvMediaStatus
SetIntValue(uint32_t i2cDevice, uint32_t sensorAddress, char *cmdStr, 
    char *arg)
{
    uint16_t cmdBody[4];
    uint32_t param = (uint32_t)strtol(arg, NULL, 16);
    _StringToCommand(cmdBody, cmdStr);

    BuildCommand(cmdBody, &param, _cmd);
    return SendCommand(i2cDevice, sensorAddress, _cmd);
}

NvMediaStatus
GetFPS(uint32_t i2cDevice, uint32_t sensorAddress, uint32_t *fps) {
    uint16_t cmdBody[4] = {0x00, 0x0E, 0x00, 0x07};

    return RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody, fps);
}