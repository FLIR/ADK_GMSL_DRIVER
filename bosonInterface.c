#include <string.h>

#include "log_utils.h"
#include "os_common.h"
#include "testutil_i2c.h"
#include "helpers.h"

#include "bosonInterface.h"

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

static uint16_t _charsToEscape[3] = {0x8E, 0x9E, 0xAE};
static uint16_t _escapeChar = 0x9E;
static uint16_t _cmdStart[7] = {0x902, 0x8E, 0x00, 0x12, 0xC0, 0xFF, 0xEE};
static uint16_t _cmdEnd[2] = {0xAE, 0x900};

static void
_EscapeCmd(uint16_t *cmd) {
    uint16_t tempCmd[64];
    uint32_t ei = 0;
    uint32_t i;
    uint32_t startLen = sizeof(_cmdStart) / sizeof(_cmdStart[0]);

    // copy start of command over
    memcpy(tempCmd, cmd, sizeof(_cmdStart));

    // loop over command but skip start characters
    for (i = startLen; i < 64; i++) {
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
_UnescapeResponse(uint8_t *resp, uint32_t length) {
    uint8_t tempResp[length];
    uint32_t ei = 0;
    for (size_t i = 0; i < length; i++) {
        if(resp[i+ei] == _escapeChar) {
            ei++;
            resp[i+ei] += 0xD;
        }

        tempResp[i] = resp[i+ei];
    }
    memcpy(resp, tempResp, length * sizeof(uint8_t));
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
_SendSingleCommand(uint32_t i2cDevice, uint32_t sensorAddress, uint16_t cmd) {
    I2cHandle handle = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint8_t instruction[2] = {cmd >> 8, cmd & 0xFF};

    testutil_i2c_open(i2cDevice, &handle);
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
_ReceiveHelper(uint32_t i2cDevice, uint32_t sensorAddress, uint8_t reg, 
    uint8_t *buffer)
{
    I2cHandle handle = NULL;
    unsigned char respByte;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    int startIdx = -1;
    uint32_t cmdStatus;

    testutil_i2c_open(i2cDevice, &handle);
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

finally:
    testutil_i2c_close(handle);

    return status;
}


void
BuildCommand(uint16_t *cmdBody, uint32_t *value, uint16_t *outCmd) {
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
        uint8_t valBytes[4];

        LsbToMsbArr(valBytes, *value);
        for (size_t i = 0; i < 4; i++) {
            totalCmd[cmdIdx+i] = valBytes[i];
        }
        
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
SendCommand(uint32_t i2cDevice, uint32_t sensorAddress, uint16_t *cmd) {
    I2cHandle handle = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint32_t cmdEndLength = sizeof(_cmdEnd) / sizeof(_cmdEnd[0]);
    uint32_t i;

    uint16_t tempCmd[64];

    testutil_i2c_open(i2cDevice, &handle);
    if(!handle) {
        LOG_ERR("%s: Failed to open handle with id %u\n", __func__,
            sensorAddress);
        return NVMEDIA_STATUS_ERROR;
    }

    memcpy(tempCmd, cmd, 64 * sizeof(uint16_t));
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
ReceiveData(uint32_t i2cDevice, uint32_t sensorAddress, uint8_t reg, 
    uint32_t *response)
{
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint8_t buffer[64];  

    status = _ReceiveHelper(i2cDevice, sensorAddress, reg, buffer);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error receiving data", __func__);
        return status;
    }

    _UnescapeResponse(&buffer[13], 4);
    MsbToLsb32(response, &buffer[13]);

    return status;
}

NvMediaStatus
ReceiveStringData(uint32_t i2cDevice, uint32_t sensorAddress, uint8_t reg, 
    char *response, uint32_t length)
{
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint8_t buffer[64];  

    status = _ReceiveHelper(i2cDevice, sensorAddress, reg, buffer);
    if(status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Error receiving data", __func__);
        return status;
    }

    _UnescapeResponse(&buffer[13], length);
    memcpy(response, &buffer[13], length * sizeof(char));

    return status;
}

void
ResetI2CBuffer(uint32_t i2cDevice, uint32_t sensorAddress) {
    uint16_t off = 0x0A02;
    uint16_t on = 0x0A00;
    _SendSingleCommand(i2cDevice, sensorAddress, off);
    nvsleep(10000);
    _SendSingleCommand(i2cDevice, sensorAddress, on);
}