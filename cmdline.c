/* Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log_utils.h"
#include "cmdline.h"

static void
PrintUsage(void)
{
    NvMediaIDPDeviceParams outputs[MAX_OUTPUT_DEVICES];
    int outputDevicesNum = 0, i;

    NvMediaIDPQuery(&outputDevicesNum, outputs);

    LOG_MSG("Usage: nvmimg_cc [options]\n");
    LOG_MSG("\nAvailable command line options:\n");
    LOG_MSG("-h                Print usage\n");
    LOG_MSG("-v [level]        Logging Level. Default = 0\n");
    LOG_MSG("                  0: Errors, 1: Warnings, 2: Info, 3: Debug\n");
    LOG_MSG("                  Default: 0\n");
    LOG_MSG("-d [n]            Set display ID\n");
    LOG_MSG("                  Available display devices (%d):\n", outputDevicesNum);
    for (i = 0; i < outputDevicesNum; i++) {
        LOG_MSG("                  Display ID: %d\n", outputs[i].displayId);
    }

    LOG_MSG("-n [frames]       Number of frames to Capture.\n");
    LOG_MSG("-f [file-prefix]  Save raw files. Provide pre-fix for each file to save\n");
    LOG_MSG("-z [n]            Set display window depth [0-255]\n");
    LOG_MSG("-p [position]     Window position. Default: full screen size\n");
    LOG_MSG("-s [n]            Set frame number to start capturing images\n");
    LOG_MSG("-b [n]            Set buffer pool size\n");
    LOG_MSG("                  Default: %d Maximum: %d\n",MIN_BUFFER_POOL_SIZE,NVMEDIA_MAX_CAPTURE_FRAME_BUFFERS);
    LOG_MSG("-wrregs [file]    File name of register script to write to sensor\n");
    LOG_MSG("-rdregs [file]    File name of register dump from sensor\n");
    LOG_MSG("--vc_enable       Enable virtual channels for capturing the frames\n");
    LOG_MSG("                  VCs are always enabled. This option will be deprecated\n");
    LOG_MSG("--crystalF [MHz]  Crystal Frequency in MHz.\n");
    LOG_MSG("                  Default = 24 MHz\n");
    LOG_MSG("--wait [n]        Wait for n frames before capturing the next frame(s)\n");
    LOG_MSG("--miniburst [n]   Capture n frames between wait periods.\n");
    LOG_MSG("                  Default = 1\n");
    LOG_MSG("                  Valid only when --wait is used\n");
    LOG_MSG("\nValid Script File Commands:\n");
    LOG_MSG("; Delay [n](ms|us)         Delay between register writes in ms/us\n");
    LOG_MSG("; I2C [channel]            Open I2C channel for writing registers\n");
    LOG_MSG("; Wait for frame [i]       Waits for frame i to be captured before writing subsequent registers\n");
    LOG_MSG("; End frame [i] registers  Marks the end of registers to write after frame i has been captured\n");
    LOG_MSG("                           Mandatory if Wait for frame has been used\n");
    LOG_MSG("# [comment]                Symbol for using comments in the script file\n");
    LOG_MSG("\nSensor Calibration Commands:\n");
    LOG_MSG(" To get specific sensor calibration commads, please specify '-sensor [name] -h'\n");
    LOG_MSG(" To get log info about sensor calibration actual setting, please specify '-v 2'\n");
}

NvMediaStatus
ParseArgs(int argc,
          char *argv[],
          TestArgs *allArgs)
{
    int i = 0;
    uint32_t j = 0;
    NvMediaBool bLastArg = NVMEDIA_FALSE;
    NvMediaBool bDataAvailable = NVMEDIA_FALSE;
    uint32_t x, y, w, h;
    NvMediaBool foundArg = NVMEDIA_FALSE;
    NvMediaStatus status;
    NvMediaBool bHelpArg = NVMEDIA_FALSE;
    NvMediaBool bSensorArg = NVMEDIA_FALSE;

    // Default parameters
    allArgs->numSensors = 1;
    allArgs->numLinks = 0;
    allArgs->numVirtualChannels = 1;
    allArgs->crystalFrequency = 24;
    allArgs->bufferPoolSize = MIN_BUFFER_POOL_SIZE;
    allArgs->useVirtualChannels = NVMEDIA_TRUE;

    allArgs->windowId = 1;

    if (argc < 2) {
        PrintUsage();
        return NVMEDIA_STATUS_ERROR;
    }

    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            // Check if this is the last argument
            bLastArg = ((argc - i) == 1);

            // Check if there is data available to be parsed
            bDataAvailable = (!bLastArg) && !(argv[i+1][0] == '-');

            if (!strcasecmp(argv[i], "-h"))
                bHelpArg = NVMEDIA_TRUE;
            else if (!strcasecmp(argv[i], "-v")) {
                allArgs->logLevel = LEVEL_DBG;
                if (bDataAvailable) {
                    allArgs->logLevel = atoi(argv[++i]);
                    if (allArgs->logLevel > LEVEL_DBG) {
                        printf("Invalid logging level chosen (%d)\n",
                               allArgs->logLevel);
                        printf("Setting logging level to LEVEL_ERR (0)\n");
                        allArgs->logLevel = LEVEL_ERR;
                    }
                }
                SetLogLevel((enum LogLevel)allArgs->logLevel);
            }
        }

        if (bHelpArg) {
            PrintUsage();
            if (bSensorArg)
                allArgs->sensorInfo->PrintSensorCaliUsage();
            return NVMEDIA_STATUS_ERROR;
        }
    }

    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            // Check if this is the last argument
            bLastArg = ((argc - i) == 1);

            // Check if there is data available to be parsed
            bDataAvailable = (!bLastArg) && !(argv[i+1][0] == '-');

            if (!strcasecmp(argv[i], "-v")) {
                if (bDataAvailable)
                    i++;
            } else if (!strcasecmp(argv[i], "-sensor")) {
                if (bDataAvailable)
                    i++;
            } else if (!strcasecmp(argv[i], "-h")) {
                if (bDataAvailable)
                    i++;
            } else if (!strcasecmp(argv[i], "-wrregs")) {
                if (argv[i + 1] && argv[i + 1][0] != '-') {
                    allArgs->wrregs.isUsed = NVMEDIA_TRUE;
                    strncpy(allArgs->wrregs.stringValue, argv[++i], MAX_STRING_SIZE);
                } else {
                    LOG_ERR("-wrregs must be followed by registers script file name\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-rdregs")) {
                if (argv[i + 1] && argv[i + 1][0] != '-') {
                    allArgs->rdregs.isUsed = NVMEDIA_TRUE;
                    strncpy(allArgs->rdregs.stringValue, argv[++i], MAX_STRING_SIZE);
                } else {
                    LOG_ERR("-rdregs must be followed by registers script file name\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-f")) {
                allArgs->useFilePrefix = NVMEDIA_TRUE;
                if (argv[i + 1] && argv[i + 1][0] != '-') {
                    strncpy(allArgs->filePrefix, argv[++i], MAX_STRING_SIZE);
                } else {
                    LOG_ERR("-f must be followed by a file prefix string\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-n")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->frames.isUsed = NVMEDIA_TRUE;
                    allArgs->frames.uIntValue = atoi(arg);
                } else {
                    LOG_ERR("-n must be followed by number of frames to capture\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-d")) {
                if (bDataAvailable) {
                    allArgs->displayIdUsed = NVMEDIA_TRUE;
                    if ((sscanf(argv[++i], "%u", &allArgs->displayId) != 1)) {
                        LOG_ERR("Bad display id: %s\n", argv[i]);
                        return NVMEDIA_STATUS_ERROR;
                    }
                }
                allArgs->displayEnabled = NVMEDIA_TRUE;
            } else if (!strcasecmp(argv[i], "-z")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->depth = atoi(arg);
                } else {
                    LOG_ERR("-z must be followed by depth value\n");
                    return NVMEDIA_STATUS_ERROR;
                }
                if (allArgs->depth > 255) {
                    LOG_WARN("Bad depth value: %d. Using default value: 1\n",
                             allArgs->depth);
                    allArgs->depth = 1;
                }
            } else if (!strcasecmp(argv[i], "-p")) {
                if (bDataAvailable) {
                    if ((sscanf(argv[++i], "%u:%u:%u:%u", &x, &y, &w, &h)
                       != 4)) {
                        LOG_ERR("Bad resolution: %s\n", argv[i]);
                        return NVMEDIA_STATUS_BAD_PARAMETER;
                    }
                    allArgs->position.x0 = x;
                    allArgs->position.y0 = y;
                    allArgs->position.x1 = x + w;
                    allArgs->position.y1 = y + h;
                    allArgs->positionSpecifiedFlag = NVMEDIA_TRUE;
                    LOG_INFO("Output position set to: %u:%u:%u:%u\n",
                             x, y, x + w, y + h);
                } else {
                    LOG_ERR("-p must be followed by window position x0:x1:W:H\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-s")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->numFramesToSkip = atoi(arg);
                } else {
                    LOG_ERR("-s must be followed by frame number at which capture starts\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "-b")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->bufferPoolSize = atoi(arg);
                    if (allArgs->bufferPoolSize < MIN_BUFFER_POOL_SIZE) {
                        allArgs->bufferPoolSize = MIN_BUFFER_POOL_SIZE;
                        LOG_WARN("Buffer pool size is too low. Using default of %u\n",
                                 MIN_BUFFER_POOL_SIZE);
                    } else if (allArgs->bufferPoolSize > MAX_BUFFER_POOL_SIZE) {
                        allArgs->bufferPoolSize = MAX_BUFFER_POOL_SIZE;
                        LOG_WARN("Buffer pool size is too high. Using max of %u\n",
                                 MAX_BUFFER_POOL_SIZE);
                    }
                } else {
                    LOG_ERR("-b must be followed by buffer pool size\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "--wait")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->numFramesToWait= atoi(arg);
                } else {
                    LOG_ERR("--wait must be followed by number of frames to wait in between capture\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "--miniburst")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->numMiniburstFrames= atoi(arg);
                } else {
                    LOG_ERR("--miniburst must be followed by number of frames to capture after a wait period\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "--crystalF")) {
                if (bDataAvailable) {
                    char *arg = argv[++i];
                    allArgs->crystalFrequency = atoi(arg);
                } else {
                    LOG_ERR("--crystalF must be followed by crystal frequency in MHz\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "--aggregate")) {
                allArgs->useAggregationFlag = NVMEDIA_TRUE;
                if (bDataAvailable) {
                    if ((sscanf(argv[++i], "%u", &allArgs->numSensors) != 1)) {
                        LOG_ERR("Bad siblings number: %s\n", argv[i]);
                        return NVMEDIA_STATUS_ERROR;
                    }
                } else {
                    LOG_ERR("--aggregate must be followed by number of images to aggregate\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else if (!strcasecmp(argv[i], "--vc_enable")) {
                allArgs->useVirtualChannels= NVMEDIA_TRUE;
            } else if (!strcasecmp(argv[i], "--settings")) {
                if (argv[i + 1] && argv[i + 1][0] != '-') {
                    allArgs->rtSettings.isUsed = NVMEDIA_TRUE;
                    strncpy(allArgs->rtSettings.stringValue, argv[++i], MAX_STRING_SIZE);
                } else {
                    LOG_ERR("--settings must be followed by name of the file containing runtime settings\n");
                    return NVMEDIA_STATUS_ERROR;
                }
            } else {
                foundArg = NVMEDIA_FALSE;
                if (allArgs->sensorInfo) {
                    for (j = 0; j < allArgs->sensorInfo->numSupportedArgs; j++) {
                        if (!strcasecmp(argv[i], allArgs->sensorInfo->supportedArgs[j])) {
                            foundArg = NVMEDIA_TRUE;
                            break;
                        }
                    }
                }
                if (!foundArg) {
                    LOG_ERR("Unsupported option encountered: %s\n", argv[i]);
                    return NVMEDIA_STATUS_ERROR;
                }
                allArgs->calibrateSensorFlag = NVMEDIA_TRUE;
                if (bDataAvailable)
                    i++;
            }
        }

        if (allArgs->sensorInfo) {
            // Process sensor properties
            allArgs->sensorProperties = calloc(1, allArgs->sensorInfo->sizeOfSensorProperties);
            if(!allArgs->sensorProperties) {
                LOG_ERR("%s: Failed to create sensor properties.\n", __func__);
                return NVMEDIA_STATUS_OUT_OF_MEMORY;
            }

            // Process calibration commands
            status = allArgs->sensorInfo->ProcessCmdline(argc, argv, allArgs->sensorProperties);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("Failed to process calibration parameters\n");
                return NVMEDIA_STATUS_ERROR;
            }
        }
    }

    if (allArgs->numSensors > NVMEDIA_MAX_AGGREGATE_IMAGES) {
        LOG_WARN("Max aggregate images is: %u\n",
                 NVMEDIA_MAX_AGGREGATE_IMAGES);
        allArgs->numSensors = NVMEDIA_MAX_AGGREGATE_IMAGES;
    }

    if (allArgs->numLinks > 0) {
        if (allArgs->numLinks > allArgs->numSensors) {
            LOG_ERR("The number of enabled links with cam_enable option can't be larger than the number with aggregate option\n");
            return NVMEDIA_STATUS_ERROR;
        }
        allArgs->numSensors = allArgs->numLinks;
    }

    // Set the same capture set for all virtual channels
    // TBD: Add unique capture set for each vc once there is hw support
    if (allArgs->useVirtualChannels) {
        allArgs->numVirtualChannels = allArgs->numSensors;
        for (j = 0; j < allArgs->numSensors; j++) {
            allArgs->config[j].isUsed = NVMEDIA_TRUE;
            allArgs->config[j].uIntValue = allArgs->config[0].uIntValue;
        }
    }

    if (allArgs->numMiniburstFrames && !allArgs->numFramesToWait) {
        LOG_ERR("--miniburst cannot be used without --wait option\n");
        return NVMEDIA_STATUS_ERROR;
    }

    // Set to default of 1 to capture every frame
    if (!allArgs->numMiniburstFrames)
        allArgs->numMiniburstFrames = 1;

    return NVMEDIA_STATUS_OK;
}
