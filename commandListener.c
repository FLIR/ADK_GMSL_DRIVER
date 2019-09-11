#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "os_common.h"

#include "helpers.h"
#include "capture.h"
#include "commandListener.h"
#include "opencvConnector.h"

static uint16_t CRC16_XMODEM_TABLE[256] = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

static uint16_t _escapeChar = 0x9E;
static uint16_t _charsToEscape[3] = {0x8E, 0x9E, 0xAE};
static uint16_t _cmdStart[7] = {0x902, 0x8E, 0x00, 0x12, 0xC0, 0xFF, 0xEE};
static uint16_t _cmdEnd[2] = {0xAE, 0x900};
static uint16_t _ffcCommand[4] = {0x00, 0x05, 0x00, 0x07};
static uint16_t _getSNCommand[4] = {0x00, 0x05, 0x00, 0x02};
static uint16_t _changePaletteCommand[4] = {0x00, 0x0B, 0x00, 0x03};
static uint16_t _setFFCMode[4] = {0x00, 0x05, 0x00, 0x12};
static uint16_t _getPacking[4] = {0x00, 0x04, 0x00, 0x06};
static uint16_t _getPalette[4] = {0x00, 0x0B, 0x00, 0x04};

static void
_GetCRC(uint16_t *data, uint32_t length, uint16_t *outCrc) {
    uint16_t crc = 0x1d0f;
    uint8_t byte;

    for (size_t i = 0; i < length; i++) {
        byte = data[i] & 0xff;
        crc = ((crc << 8) & 0xff00) ^ CRC16_XMODEM_TABLE[((crc >> 8) & 0xff) 
            ^ byte];
    }
    crc = crc & 0xffff;

    outCrc[0] = crc >> 8;
    outCrc[1] = crc & 0xff;
}

static void
_EscapeCmd(uint16_t *cmd, uint32_t *length) {
    uint16_t tempCmd[64];
    uint32_t ei = 0;

    // loop over command but skip start and end characters
    for (size_t i = 2; i < (int)(*length - 2); i++) {
        tempCmd[i+ei] = cmd[i];
        for (size_t j = 0; j < 3; j++) {
            if(_charsToEscape[j] == cmd[i]) {
                tempCmd[i+ei] = _escapeChar;
                ei++;
                tempCmd[i+ei] = cmd[i] - 0xD;
                break;
            }
        }
    }

    *length += ei;
    memcpy(cmd, tempCmd, *length * sizeof(uint8_t));    
}

static void
_CreateCommand(uint16_t *cmdBody, uint16_t *outCmd, uint32_t *length) {
    // stop spooling, send start flag and arbitrary start data
    uint16_t totalCmd[64];
    uint16_t crc[2];
    uint32_t cmdIdx = 0;
    
    //copy start of command
    memcpy(totalCmd, _cmdStart, sizeof(_cmdStart));
    cmdIdx += sizeof(_cmdStart) / sizeof(_cmdStart[0]);
    // copy data type and value
    memcpy(&totalCmd[cmdIdx], cmdBody, sizeof(cmdBody));
    cmdIdx += sizeof(cmdBody) / sizeof(cmdBody[0]);
    // sending a command so status is 4 0xFF bytes
    for (size_t i = cmdIdx; i < cmdIdx + 4; i++) {
        totalCmd[i] = 0xFF;
    }
    cmdIdx += 4;

    _GetCRC(&totalCmd[2], cmdIdx - 2, crc);
    memcpy(&totalCmd[cmdIdx], crc, sizeof(crc));
    cmdIdx += sizeof(crc) / sizeof(crc[0]);

    // send stop flag and start spooling
    memcpy(&totalCmd[cmdIdx], _cmdEnd, sizeof(_cmdEnd));
    cmdIdx += sizeof(_cmdEnd) / sizeof(_cmdEnd[0]);

    memcpy(outCmd, totalCmd, sizeof(totalCmd));
    *length = cmdIdx;
}

static void
_CreateParamCommand(uint16_t *cmdBody, uint32_t value, uint16_t *outCmd) {
    uint16_t totalCmd[23];
    uint16_t crc[2];

    memcpy(totalCmd, _cmdStart, sizeof(_cmdStart));
    memcpy(&totalCmd[7], cmdBody, sizeof(cmdBody));
    for (size_t i = 11; i < 15; i++) {
        totalCmd[i] = 0xFF;
    }
    // copy value MSB
    for (size_t i = 0; i < 4; i++) {
        totalCmd[i + 15] = (value >> (24 - (8 * i))) & 0xFF;
    }
    
    memcpy(&totalCmd[15], &value, sizeof(value));
    _GetCRC(&totalCmd[2], 17, crc);
    memcpy(&totalCmd[19], crc, sizeof(crc));
    memcpy(&totalCmd[21], _cmdEnd, sizeof(_cmdEnd));

    memcpy(outCmd, totalCmd, sizeof(totalCmd));
}

static NvMediaStatus
_SendCommand(uint32_t sensorAddress, uint16_t *cmd, uint32_t length) {
    I2cHandle handle = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    // TODO: get I2C port from context 
    testutil_i2c_open(0, &handle);
    if(!handle) {
        LOG_ERR("%s: Failed to open handle with id %u\n", __func__,
            sensorAddress);
        return NVMEDIA_STATUS_ERROR;
    }

    _EscapeCmd(cmd, &length);

    for (size_t i = 0; i < length; i++) {
        uint8_t instruction[2] = {cmd[i] >> 8, cmd[i] & 0xFF};

        if(testutil_i2c_write_subaddr(handle, sensorAddress, 
            &instruction, 2)) 
        {
            LOG_ERR("%s: Failed to write to I2C %02x %02x %02x",
                __func__, sensorAddress,
                instruction[0],
                instruction[1]);
            status = NVMEDIA_STATUS_ERROR;
            break;
        }
    }

    testutil_i2c_close(handle);
    
    return status;
}

static NvMediaStatus
_SendSingleCommand(uint32_t sensorAddress, uint16_t cmd) {
    I2cHandle handle = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint8_t instruction[2] = {cmd >> 8, cmd & 0xFF};

    // TODO: get I2C port from context 
    testutil_i2c_open(0, &handle);
    if(!handle) {
        LOG_ERR("%s: Failed to open handle with id %u\n", __func__,
            sensorAddress);
        status = NVMEDIA_STATUS_ERROR;
        goto finally;
    }

    if(testutil_i2c_write_subaddr(handle, sensorAddress, 
            &instruction, 2)) 
    {
        LOG_ERR("%s: Failed to write to I2C %02x %02x %02x",
            __func__, sensorAddress,
            instruction[0],
            instruction[1]);
        status = NVMEDIA_STATUS_ERROR;
    }

finally:
    testutil_i2c_close(handle);

    return status;
}

static NvMediaStatus
_ReceiveData(uint32_t sensorAddress, uint8_t reg, uint32_t *response) {
    I2cHandle handle = NULL;
    unsigned char respByte;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    int startIdx = -1;
    uint32_t cmdStatus;
    uint8_t buffer[64];  

    testutil_i2c_open(0, &handle);
    if(!handle) {
        LOG_ERR("%s: Failed to open handle with id %u\n", __func__,
            sensorAddress);
        status = NVMEDIA_STATUS_ERROR;
        goto finally;
    }

    for (size_t i = 0; i < 64; i++)
    {
        testutil_i2c_read_subaddr(handle, sensorAddress, &reg, 
            sizeof(char), &respByte, sizeof(char));
        if(respByte == 0x8e) {
            startIdx = i + 1;
            continue;
        }
        if(respByte == 0xae) {
            startIdx = -1;
            break;
        }
        if(startIdx > -1) {
            buffer[i - startIdx] = respByte;
        }
    }
    // if there was never an end flag
    if(startIdx != -1) {
        LOG_ERR("%s: No termination character found", __func__);
        status = NVMEDIA_STATUS_ERROR;
        goto finally;
    }
    MsbToLsb32(&cmdStatus, &buffer[9]);
    if(cmdStatus) {
        LOG_ERR("%s: Error reading buffer - %d", __func__, cmdStatus);
        status = NVMEDIA_STATUS_ERROR;
        goto finally;
    }

    MsbToLsb32(response, &buffer[13]);
    
finally:
    testutil_i2c_close(handle);

    return status;
}

static void
_ResetI2CBuffer(uint32_t sensorAddress) {
    uint16_t off = 0x0A02;
    uint16_t on = 0x0A00;
    _SendSingleCommand(sensorAddress, off);
    nvsleep(10000);
    _SendSingleCommand(sensorAddress, on);
}

static uint32_t
_BosonThreadFunc(void *data) {
    BosonThreadCtx *threadCtx = (BosonThreadCtx *)data;
    NvMediaStatus status;
    uint32_t receivedData = 0;
    bool hasResponse;
    uint16_t cmd[64];
    uint32_t cmdLength;
    uint32_t serialNumber;

    while(!(*threadCtx->quit)) {
        hasResponse = false;

        if(threadCtx->cmd && threadCtx->cmd[0] != '\0') {
            if(!strcasecmp(threadCtx->cmd, "f")) {
                _CreateCommand(_ffcCommand, cmd, &cmdLength);
            } else if(!strcasecmp(threadCtx->cmd, "sn")) {
                serialNumber = Opencv_getSerialNumber();
                printf("%d\n", serialNumber);
                _ResetI2CBuffer(threadCtx->sensorAddress);
                _CreateCommand(_getSNCommand, cmd, &cmdLength);
                hasResponse = true;
                // goto input_done;
            } else if(!strcasecmp(threadCtx->cmd, "w")) {
                _CreateParamCommand(_changePaletteCommand, 0, cmd);
            } else if(!strcasecmp(threadCtx->cmd, "b")) {
                _CreateParamCommand(_changePaletteCommand, 1, cmd);
            } else if(!strcasecmp(threadCtx->cmd, "fa")) {
                // set FFC to auto
                _CreateParamCommand(_setFFCMode, 1, cmd);
            } else if(!strcasecmp(threadCtx->cmd, "fm")) {
                // set FFC to manual
                _CreateParamCommand(_setFFCMode, 0, cmd);
            } else if(!strcasecmp(threadCtx->cmd, "p")) {
                // get telemetry packing
                _ResetI2CBuffer(threadCtx->sensorAddress);
                _CreateCommand(_getPacking, cmd, &cmdLength);
                hasResponse = true;
            } else if(!strcasecmp(threadCtx->cmd, "c")) {
                // get color mode
                _ResetI2CBuffer(threadCtx->sensorAddress);
                _CreateCommand(_getPalette, cmd, &cmdLength);
                hasResponse = true;
            } else {
                goto input_done;
            }
            status = _SendCommand(threadCtx->sensorAddress, cmd, cmdLength);
            if(status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Unable to send I2C command", __func__);
                goto input_done;
            }
            if(hasResponse) {
                nvsleep(1000);
                status = _ReceiveData(threadCtx->sensorAddress, 0, &receivedData);
                if(status != NVMEDIA_STATUS_OK) {
                    LOG_ERR("%s: Unable to receive I2C command", __func__);
                    goto input_done;
                }
                printf("%d\n", receivedData);
            }

        input_done:
            sprintf(threadCtx->cmd, "");
        }
    }

    threadCtx->exitedFlag = NVMEDIA_TRUE;

    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
ListenerInit(NvMainContext *mainCtx) {
    NvBosonContext *bosonCtx = NULL;
    uint32_t i = 0;
    TestArgs *testArgs = mainCtx->testArgs;

    NvCaptureContext *captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT]; 
    uint32_t sensorAddress = captureCtx->captureParams.sensorAddress.uIntValue;
    
    if(!(mainCtx->ctxs[BOSON_ELEMENT] = malloc(sizeof(NvBosonContext)))) {
        LOG_ERR("%s: Failed to allocate memory for save context\n", __func__);
        return NVMEDIA_STATUS_OUT_OF_MEMORY;
    }

    bosonCtx = mainCtx->ctxs[BOSON_ELEMENT];
    memset(bosonCtx, 0, sizeof(NvBosonContext));

    // initialize context
    bosonCtx->quit = &mainCtx->quit;
    bosonCtx->numVirtualChannels = testArgs->numVirtualChannels;
    bosonCtx->cmd = mainCtx->cmd;

    for (i = 0; i < bosonCtx->numVirtualChannels; i++)
    {
        BosonThreadCtx *threadCtx = &bosonCtx->threadCtx[i];
        threadCtx->quit = bosonCtx->quit;
        threadCtx->sensorAddress = sensorAddress;
        threadCtx->cmd = bosonCtx->cmd;
        threadCtx->exitedFlag = NVMEDIA_FALSE;
    }
    
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
ListenerFini(NvMainContext *mainCtx) {
    NvBosonContext *bosonCtx;
    uint32_t i = 0;
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    if(!mainCtx) {
        return NVMEDIA_STATUS_OK;
    }
    bosonCtx = mainCtx->ctxs[BOSON_ELEMENT];
    if(!bosonCtx) {
        return NVMEDIA_STATUS_OK;
    }

    *bosonCtx->quit = NVMEDIA_TRUE;

    for (i = 0; i < bosonCtx->numVirtualChannels; i++) {
        if(bosonCtx->bosonThread[i]) {
            while(!bosonCtx->threadCtx[i].exitedFlag) {
                LOG_DBG("%s: Waiting for boson command thread %d to quit\n",
                    __func__, i);
            }

            status = NvThreadDestroy(bosonCtx->bosonThread[i]);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to destroy boson command thread %d\n",
                        __func__, i);
            }
        }
    }
    
    if(bosonCtx) {
        free(bosonCtx);
    }

    LOG_INFO("%s: BosonFini done\n", __func__);
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
ListenerProc(NvMainContext *mainCtx) {
    if (!mainCtx) {
        LOG_ERR("%s: Bad parameter\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }
    
    NvBosonContext *bosonCtx = mainCtx->ctxs[BOSON_ELEMENT];
    NvMediaStatus status= NVMEDIA_STATUS_OK;
    uint32_t i = 0;

    /* Create thread to save images */
    for (i = 0; i < bosonCtx->numVirtualChannels; i++) {
        bosonCtx->threadCtx[i].exitedFlag = NVMEDIA_FALSE;
        status = NvThreadCreate(&bosonCtx->bosonThread[i],
                                &_BosonThreadFunc,
                                (void *)&bosonCtx->threadCtx[i],
                                NV_THREAD_PRIORITY_NORMAL);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to create boson command Thread\n",
                    __func__);
            bosonCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        }
    }

    return status;
}
