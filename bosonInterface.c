#include <string.h>

#include "testutil_i2c.h"
#include "helpers.h"

#include "bosonInterface.h"

static uint16_t _charsToEscape[3] = {0x8E, 0x9E, 0xAE};
static uint16_t _escapeChar = 0x9E;
static uint16_t _cmdStart[7] = {0x902, 0x8E, 0x00, 0x12, 0xC0, 0xFF, 0xEE};
static uint16_t _cmdEnd[2] = {0xAE, 0x900};

static void
_EscapeCmd(uint16_t *cmd) {
    uint16_t tempCmd[64];
    uint32_t ei = 0;
    uint32_t i;

    // copy start of command over
    memcpy(tempCmd, cmd, 2 * sizeof(uint8_t));

    // loop over command but skip start characters
    for (i = 2; i < 64; i++) {
        if(cmd[i] == _cmdEnd[0]) {
            break;
        }

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

    // if there's no end token, just return
    if(i == 64) {
        return;
    }

    memcpy(cmd, tempCmd, (i + ei) * sizeof(uint8_t));    
}

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

void
BuildCommand(uint16_t *cmdBody, uint32_t value, uint16_t *outCmd) {
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

    if(value) {
        LsbToMsbArr(&totalCmd[cmdIdx], value);
        cmdIdx += 4;
    }

    _GetCRC(&totalCmd[2], cmdIdx - 2, crc);
    memcpy(&totalCmd[cmdIdx], crc, sizeof(crc));
    cmdIdx += sizeof(crc) / sizeof(crc[0]);

    // send stop flag and start spooling
    memcpy(&totalCmd[cmdIdx], _cmdEnd, sizeof(_cmdEnd));
    cmdIdx += sizeof(_cmdEnd) / sizeof(_cmdEnd[0]);

    memcpy(outCmd, totalCmd, sizeof(totalCmd));
}

NvMediaStatus
SendCommand(uint32_t sensorAddress, uint16_t *cmd) {
    I2cHandle handle = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint32_t cmdEndLength = sizeof(_cmdEnd) / sizeof(_cmdEnd[0]);
    uint32_t i;

    // TODO: get I2C port from context 
    testutil_i2c_open(0, &handle);
    if(!handle) {
        LOG_ERR("%s: Failed to open handle with id %u\n", __func__,
            sensorAddress);
        return NVMEDIA_STATUS_ERROR;
    }

    _EscapeCmd(cmd);
    
    for (i = 0; i < 64; i++) {
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

        if(cmd[i] == _cmdEnd[cmdEndLength-1]) {
            break;
        }
    }

    if(i == 64) {
        LOG_ERR("%s: No I2C termination character found", __func__);
        status = NVMEDIA_STATUS_ERROR;
    }    

    testutil_i2c_close(handle);

    return status;
}

NvMediaStatus
ReceiveData(uint32_t sensorAddress, uint8_t reg, uint32_t *response) {
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

void
ResetI2CBuffer(uint32_t sensorAddress) {
    uint16_t off = 0x0A02;
    uint16_t on = 0x0A00;
    _SendSingleCommand(sensorAddress, off);
    nvsleep(10000);
    _SendSingleCommand(sensorAddress, on);
}