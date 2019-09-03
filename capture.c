/* Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "capture.h"
#include "os_common.h"
#include "helpers.h"
#include "save.h"

static NvMediaStatus
_DetermineCaptureStatus(NvMediaBool *captureFlag,
                        uint32_t frameNumber,
                        uint32_t numFramesToSkip,
                        uint32_t numFramesToWait,
                        uint32_t numMiniburstFrames)
{
    uint32_t offsetFrameNumber;
    uint32_t i;

    if (!captureFlag)
        return NVMEDIA_STATUS_BAD_PARAMETER;

    /* Frames in the beginning need to be skipped */
    if (frameNumber < numFramesToSkip) {
        *captureFlag = NVMEDIA_FALSE;
        return NVMEDIA_STATUS_OK;
    }

    /* Always capture frames if frame wait is 0 */
    if (!numFramesToWait) {
        *captureFlag = NVMEDIA_TRUE;
        return NVMEDIA_STATUS_OK;
    }

    offsetFrameNumber = frameNumber - numFramesToSkip;
    i = offsetFrameNumber % (numMiniburstFrames + numFramesToWait);

    /* Capturing mini burst frames */
    if (i < numMiniburstFrames)
        *captureFlag = NVMEDIA_TRUE;
    /* Waiting for frames to be captured */
    else
        *captureFlag = NVMEDIA_FALSE;

    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
_WriteCommandsToFile(FILE *fp,
                     I2cCommands *allCommands)
{
    uint32_t i = 0;

    for (i = 0; i < allCommands->numCommands; i++) {
        Command *cmd = &allCommands->commands[i];
        switch (cmd->commandType) {
            case DELAY:
            case I2C_DEVICE:
            case I2C_ERR:
            case SECTION_START:
            case SECTION_STOP:
                /* Do nothing */
                break;
            case WRITE_REG_1:
            case READ_REG_1:
                fprintf(fp, "%02x %02x %02x\n", cmd->deviceAddress << 1,
                        cmd->buffer[0], cmd->buffer[1]);
                break;
            case WRITE_REG_2:
            case READ_REG_2:
                fprintf(fp, "%02x %02x%02x %02x\n",
                        cmd->deviceAddress << 1, cmd->buffer[0],
                        cmd->buffer[1], cmd->buffer[2]);
                break;
            default:
                LOG_ERR("%s: Unknown command type encountered\n",
                        __func__);
                fclose(fp);
                return NVMEDIA_STATUS_ERROR;
        }
    }
    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
_ReadSensorRegisters(NvCaptureContext *ctx,
                     char *fileName)
{
    NvMediaStatus status;
    FILE *fp = fopen(fileName, "w");

    if (!fp) {
        LOG_ERR("%s: Failed to open file \"%s\"\n",__func__, fileName);
        return NVMEDIA_STATUS_ERROR;
    }

    /* Get register values from I2C for parsed commands */
    status = I2cProcessCommands(&ctx->parsedCommands,
                                I2C_READ,
                                ctx->i2cDeviceNum);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to read to registers over I2C\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    /* Get register values from I2C for sensor settings */
    status = I2cProcessCommands(&ctx->settingsCommands,
                                I2C_READ,
                                ctx->i2cDeviceNum);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to read to registers over I2C\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    /* Store values to file */

    fprintf(fp, "%s\n", "#All Registers");
    status = _WriteCommandsToFile(fp, &ctx->parsedCommands);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to write parsed registers to file.\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    fprintf(fp, "%s\n", "#Sensor Settings");
    status = _WriteCommandsToFile(fp, &ctx->settingsCommands);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to write settings registers to file.\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    fclose(fp);
    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
_ReadDeserRegisters(NvCaptureContext *captureCtx)
{
    I2cHandle i2cHandle;
    int i;
    uint8_t dbg_val[6] = "";

    if (testutil_i2c_open(captureCtx->i2cDeviceNum,
                         &i2cHandle) < 0) {
        printf("%s: i2c_open() failed\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    printf("\nDeserializer registers:\n-------------------\n");
    for (i = 0; i < 256; i++)
    {
        dbg_val[0] = (uint8_t)i;
        dbg_val[1] = 0;

        if (testutil_i2c_read_subaddr(i2cHandle,
                                     captureCtx->captureParams.deserAddress.uIntValue,
                                     dbg_val, 1, &dbg_val[1], 1) < 0) {
            printf("DEBUG: testutil_i2c_read_subaddr() failed to \
                    read register %d\n", i);
            return NVMEDIA_STATUS_ERROR;
        }
        else
            printf("DEBUG: %d = \t%02X\n", i, dbg_val[1]);
    }
    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
_SetInterfaceType(CaptureConfigParams *captureParams,
                  NvMediaICPInterfaceType *interfaceType,
                  NvMediaICPCsiPhyMode *phyMode)
{
    char *interface = captureParams->interface.stringValue;
    *phyMode = NVMEDIA_ICP_CSI_DPHY_MODE;

    /* Set interface type */
    if (!strcasecmp(interface,"csi-a"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_A;
    else if (!strcasecmp(interface,"csi-b"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_B;
    else if (!strcasecmp(interface,"csi-c"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_C;
    else if (!strcasecmp(interface,"csi-d"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_D;
    else if (!strcasecmp(interface,"csi-e"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_E;
    else if (!strcasecmp(interface,"csi-f"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_F;
    else if (!strcasecmp(interface,"csi-g"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_G;
    else if (!strcasecmp(interface,"csi-h"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_H;
    else if (!strcasecmp(interface,"csi-ab"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_AB;
    else if (!strcasecmp(interface,"csi-cd"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_CD;
    else if (!strcasecmp(interface,"csi-ef"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_EF;
    else if (!strcasecmp(interface,"csi-gh"))
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_GH;
    else if (!strcasecmp(interface,"trio-ab")) {
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_AB;
        *phyMode = NVMEDIA_ICP_CSI_CPHY_MODE;
    }
    else if (!strcasecmp(interface,"trio-cd")) {
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_CD;
        *phyMode = NVMEDIA_ICP_CSI_CPHY_MODE;
    }
    else if (!strcasecmp(interface,"trio-ef")) {
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_EF;
        *phyMode = NVMEDIA_ICP_CSI_CPHY_MODE;
    }
    else if (!strcasecmp(interface,"trio-gh")) {
        *interfaceType = NVMEDIA_IMAGE_CAPTURE_CSI_INTERFACE_TYPE_CSI_GH;
        *phyMode = NVMEDIA_ICP_CSI_CPHY_MODE;
    }
    else {
        LOG_ERR("%s: Bad interface type specified: %s \n", __func__, interface);
        return NVMEDIA_STATUS_ERROR;
    }
    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
_SetICPSettings(CaptureThreadCtx *ctx,
                NvMediaICPSettings *icpSettings,
                CaptureConfigParams *captureParams,
                NvMediaICPInterfaceType interfaceType,
                NvMediaICPCsiPhyMode phyMode,
                TestArgs *testArgs)
{
    uint32_t width = 0 , height = 0;
    char *inputFormat = NULL;

    if (sscanf(captureParams->resolution.stringValue, "%ux%u",
               &width,
               &height) != 2) {
        LOG_ERR("%s: Invalid input resolution %s\n", __func__,
                captureParams->resolution.stringValue);
        return NVMEDIA_STATUS_BAD_PARAMETER;
    }
    NVM_SURF_FMT_DEFINE_ATTR(surfFormatAttrs);

    /* Set input and surface format */
    inputFormat = captureParams->inputFormat.stringValue;
    if (!strcasecmp(inputFormat, "raw8")) {
        ctx->inputFormat.inputFormatType = NVMEDIA_IMAGE_CAPTURE_INPUT_FORMAT_TYPE_RAW8;
        ctx->inputFormat.bitsPerPixel = NVMEDIA_BITS_PER_PIXEL_8;
        NVM_SURF_FMT_SET_ATTR_RAW(surfFormatAttrs,RGGB,UINT,8,PL);
        surfFormatAttrs[NVM_SURF_ATTR_COMPONENT_ORDER].value +=  captureParams->pixelOrder.uIntValue;
        ctx->rawBytesPerPixel = 1;
        ctx->pixelOrder = captureParams->pixelOrder.uIntValue;
    } else if (!strcasecmp(inputFormat, "raw12")) {
        ctx->inputFormat.inputFormatType = NVMEDIA_IMAGE_CAPTURE_INPUT_FORMAT_TYPE_RAW12;
        ctx->inputFormat.bitsPerPixel = NVMEDIA_BITS_PER_PIXEL_12;
        NVM_SURF_FMT_SET_ATTR_RAW(surfFormatAttrs,RGGB,UINT,12,PL);
        surfFormatAttrs[NVM_SURF_ATTR_COMPONENT_ORDER].value +=  captureParams->pixelOrder.uIntValue;
        ctx->rawBytesPerPixel = 2;
        ctx->pixelOrder =  captureParams->pixelOrder.uIntValue;
    } else if (!strcasecmp(inputFormat, "raw16")) {
        ctx->inputFormat.inputFormatType = NVMEDIA_IMAGE_CAPTURE_INPUT_FORMAT_TYPE_RAW16;
        ctx->inputFormat.bitsPerPixel = NVMEDIA_BITS_PER_PIXEL_16;
        NVM_SURF_FMT_SET_ATTR_RAW(surfFormatAttrs,RGGB,UINT,16,PL);
        surfFormatAttrs[NVM_SURF_ATTR_COMPONENT_ORDER].value += captureParams->pixelOrder.uIntValue;
        ctx->rawBytesPerPixel = 2;
        ctx->pixelOrder =  captureParams->pixelOrder.uIntValue;
    } else {
        LOG_ERR("%s: Bad input format specified: %s \n",
                __func__, inputFormat);
        return NVMEDIA_STATUS_BAD_PARAMETER;
    }
    ctx->surfType = NvMediaSurfaceFormatGetType(surfFormatAttrs, NVM_SURF_FMT_ATTR_MAX);

    /* Set NvMediaICPSettings */
    icpSettings->interfaceType = interfaceType;
    memcpy(&icpSettings->inputFormat, &ctx->inputFormat, sizeof(NvMediaICPInputFormat));
    icpSettings->width = (testArgs->useVirtualChannels)?
                          width : width * testArgs->numSensors;

    icpSettings->height = height;
    icpSettings->startX = 0;
    icpSettings->startY = 0;
    icpSettings->embeddedDataType = captureParams->emb.isUsed;
    icpSettings->embeddedDataLines = (captureParams->emb.isUsed == NVMEDIA_TRUE) ?
                                        captureParams->emb.uIntValue : 0;
    icpSettings->interfaceLanes = captureParams->csiLanes.uIntValue;
    icpSettings->surfaceType = ctx->surfType;
    icpSettings->phyMode = phyMode;

    /* Set SurfaceAllocAttrs */
    ctx->surfAllocAttrs[0].type = NVM_SURF_ATTR_WIDTH;
    ctx->surfAllocAttrs[0].value = icpSettings->width;
    ctx->surfAllocAttrs[1].type = NVM_SURF_ATTR_HEIGHT;
    ctx->surfAllocAttrs[1].value = icpSettings->height;
    ctx->surfAllocAttrs[2].type = NVM_SURF_ATTR_EMB_LINES_TOP;
    ctx->surfAllocAttrs[2].value = icpSettings->embeddedDataLines;
    ctx->surfAllocAttrs[3].type = NVM_SURF_ATTR_EMB_LINES_BOTTOM;
    ctx->surfAllocAttrs[3].value = 0;
    ctx->surfAllocAttrs[4].type = NVM_SURF_ATTR_CPU_ACCESS;
    ctx->surfAllocAttrs[4].value = NVM_SURF_ATTR_CPU_ACCESS_CACHED;
    ctx->surfAllocAttrs[5].type = NVM_SURF_ATTR_ALLOC_TYPE;
    ctx->surfAllocAttrs[5].value = NVM_SURF_ATTR_ALLOC_ISOCHRONOUS;
    ctx->numSurfAllocAttrs = 6;
    return NVMEDIA_STATUS_OK;
}

static uint32_t
_CaptureThreadFunc(void *data)
{
    CaptureThreadCtx *threadCtx = (CaptureThreadCtx *)data;
    uint32_t i = 0, totalCapturedFrames = 0, lastCapturedFrame = 0;
    NvMediaBool startCapture = NVMEDIA_FALSE;
    NvMediaImage *capturedImage = NULL;
    NvMediaImage *feedImage = NULL;
    NvMediaStatus status;
    uint64_t tbegin = 0, tend = 0;
    NvMediaICP *icpInst = NULL;
    uint32_t retry = 0;

    for (i = 0; i < threadCtx->icpExCtx->numVirtualGroups; i++) {
        if (threadCtx->icpExCtx->icp[i].virtualGroupId == threadCtx->virtualGroupIndex) {
            icpInst = NVMEDIA_ICP_HANDLER(threadCtx->icpExCtx,i);
            break;
        }
    }
    if (!icpInst) {
        LOG_ERR("%s: Failed to get icpInst for virtual channel %d\n", __func__,
                threadCtx->virtualGroupIndex);
        goto done;
    }

    while (!(*threadCtx->quit)) {
        status = _DetermineCaptureStatus(&startCapture,
                                         i,
                                         threadCtx->numFramesToSkip,
                                         threadCtx->numFramesToWait,
                                         threadCtx->numMiniburstFrames);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: CaptureDetermineStatus failed\n", __func__);
            *threadCtx->quit = NVMEDIA_TRUE;
            goto done;
        }

        /* Set current frame to be an offset by frames to skip */
        if (startCapture)
            threadCtx->currentFrame = i - threadCtx->numFramesToSkip;

        /* Feed all images to image capture object from the input Queue */
        while (NvQueueGet(threadCtx->inputQueue,
                          &feedImage,
                          0) == NVMEDIA_STATUS_OK) {

            status = NvMediaICPFeedFrame(icpInst,
                                         feedImage,
                                         CAPTURE_FEED_FRAME_TIMEOUT);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: %d: NvMediaICPFeedFrame failed\n", __func__, __LINE__);
                if (NvQueuePut((NvQueue *)feedImage->tag,
                               (void *)&feedImage,
                               0) != NVMEDIA_STATUS_OK) {
                    LOG_ERR("%s: Failed to put image back into capture input queue", __func__);
                    *threadCtx->quit = NVMEDIA_TRUE;
                    status = NVMEDIA_STATUS_ERROR;
                    goto done;
                }
                feedImage = NULL;
                *threadCtx->quit = NVMEDIA_TRUE;
                goto done;
            }
            feedImage = NULL;
        }

        /* Get captured frame */
        status = NvMediaICPGetFrameEx(icpInst,
                                      CAPTURE_GET_FRAME_TIMEOUT,
                                      &capturedImage);
        switch (status) {
            case NVMEDIA_STATUS_OK:
                retry = 0;
                break;
            case NVMEDIA_STATUS_TIMED_OUT:
                LOG_WARN("%s: NvMediaICPGetFrameEx timed out\n", __func__);
                if (++retry > CAPTURE_MAX_RETRY) {
                    LOG_ERR("%s: keep failing at NvMediaICPGetFrameEx for %d times\n", __func__, retry);
                    retry=0;
                }
                continue;
            case NVMEDIA_STATUS_INSUFFICIENT_BUFFERING:
                LOG_WARN("%s: NvMediaICPGetFrameEx failed as no frame buffers are available for capture."
                         "Please feed frames using NvMediaICPFeedFrame\n", __func__);
                continue;
            case NVMEDIA_STATUS_ERROR:
            default:
                LOG_ERR("%s: NvMediaICPGetFrameEx failed\n", __func__);
                *threadCtx->quit = NVMEDIA_TRUE;
                goto done;
        }

        GetTimeMicroSec(&tend);
        uint64_t td = tend - tbegin;
        if (td > 3000000) {
            threadCtx->fps = (int)(totalCapturedFrames-lastCapturedFrame)*(1000000.0/td);

            tbegin = tend;
            lastCapturedFrame = totalCapturedFrames;
            LOG_INFO("%s: VC:%d FPS=%d delta=%lld", __func__,
                     threadCtx->virtualGroupIndex, threadCtx->fps, td);
        }

        /* push the captured image onto output queue */
        if (startCapture) {
            status = NvQueuePut(threadCtx->outputQueue,
                                (void *)&capturedImage,
                                CAPTURE_ENQUEUE_TIMEOUT);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_INFO("%s: Failed to put image onto capture output queue", __func__);
                goto done;
            }

            totalCapturedFrames++;
        } else {
            status = NvQueuePut((NvQueue *)capturedImage->tag,
                                (void *)&capturedImage,
                                0);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to put image back into capture input queue", __func__);
                *threadCtx->quit = NVMEDIA_TRUE;
                goto done;
            }

        }
        capturedImage = NULL;
done:
        if (capturedImage) {
            status = NvQueuePut((NvQueue *)capturedImage->tag,
                                (void *)&capturedImage,
                                0);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to put image back into capture input queue", __func__);
                *threadCtx->quit = NVMEDIA_TRUE;
            }
            capturedImage = NULL;
        }
        i++;

        /* To stop capturing if specified number of frames are captured */
        if (threadCtx->numFramesToCapture && startCapture &&
           (totalCapturedFrames == threadCtx->numFramesToCapture))
            break;
    }

    /* Release all the frames which are fed */
    while (NvMediaICPReleaseFrame(icpInst, &capturedImage) == NVMEDIA_STATUS_OK) {
        if (capturedImage) {
            status = NvQueuePut((NvQueue *)capturedImage->tag,
                                (void *)&capturedImage,
                                0);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to put image back into input queue", __func__);
                break;
            }
        }
        capturedImage = NULL;
    }
    NvMediaICPStop(icpInst);

    LOG_INFO("%s: Capture thread exited\n", __func__);
    threadCtx->exitedFlag = NVMEDIA_TRUE;
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
CaptureInit(NvMainContext *mainCtx)
{
    NvCaptureContext *captureCtx = NULL;
    NvMediaStatus status;
    TestArgs *testArgs = mainCtx->testArgs;
    uint32_t i = 0;

    if (!mainCtx) {
        LOG_ERR("%s: Bad parameter", __func__);
        return NVMEDIA_STATUS_BAD_PARAMETER;
    }

    mainCtx->ctxs[CAPTURE_ELEMENT]= malloc(sizeof(NvCaptureContext));
    if (!mainCtx->ctxs[CAPTURE_ELEMENT]) {
        LOG_ERR("%s: Failed to allocate memory for capture context\n", __func__);
        return NVMEDIA_STATUS_OUT_OF_MEMORY;
    }

    captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT];
    memset(captureCtx, 0, sizeof(NvCaptureContext));

    /* initialize capture context */
    captureCtx->quit = &mainCtx->quit;
    captureCtx->testArgs  = testArgs;
    captureCtx->numSensors = testArgs->numSensors;
    captureCtx->numVirtualChannels = testArgs->numVirtualChannels;
    captureCtx->sensorInfo = testArgs->sensorInfo;
    captureCtx->inputQueueSize = testArgs->bufferPoolSize;
    captureCtx->crystalFrequency = testArgs->crystalFrequency;
    captureCtx->useNvRawFormat = NVMEDIA_FALSE;
    captureCtx->sensorInfo = testArgs->sensorInfo;

    /* Parse registers file */
    if (testArgs->wrregs.isUsed) {
        status = ParseRegistersFile(testArgs->wrregs.stringValue,
                                    &captureCtx->captureParams,
                                    &captureCtx->parsedCommands);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to parse register file\n",__func__);
            goto failed;
        }
    }
    captureCtx->i2cDeviceNum = captureCtx->captureParams.i2cDevice.uIntValue;
    captureCtx->calParams.i2cDevice = captureCtx->i2cDeviceNum;
    captureCtx->calParams.sensorAddress = captureCtx->captureParams.sensorAddress.uIntValue;
    captureCtx->calParams.crystalFrequency = captureCtx->crystalFrequency;

    /* Create NvMedia Device */
    captureCtx->device = NvMediaDeviceCreate();
    if (!captureCtx->device) {
        status = NVMEDIA_STATUS_ERROR;
        LOG_ERR("%s: Failed to create NvMedia device\n", __func__);
        goto failed;
    }

    /* Set NvMediaICPSettingsEx */

    status = _SetInterfaceType(&captureCtx->captureParams,
                               &captureCtx->interfaceType,
                               &captureCtx->phyMode);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to set interface type \n", __func__);
        goto failed;
    }

    captureCtx->icpSettingsEx.numVirtualGroups = testArgs->numVirtualChannels;
    captureCtx->icpSettingsEx.interfaceType = captureCtx->interfaceType;
    captureCtx->icpSettingsEx.interfaceLanes = captureCtx->captureParams.csiLanes.uIntValue;
    captureCtx->icpSettingsEx.phyMode = captureCtx->phyMode;

    for (i = 0; i < captureCtx->icpSettingsEx.numVirtualGroups; i++) {
        captureCtx->icpSettingsEx.virtualGroups[i].numVirtualChannels = 1;
        captureCtx->icpSettingsEx.virtualGroups[i].virtualChannels[0].virtualChannelIndex = i;
        status = _SetICPSettings(&captureCtx->threadCtx[i],
                                 NVMEDIA_ICP_SETTINGS_HANDLER(captureCtx->icpSettingsEx, i, 0),
                                 &captureCtx->captureParams,
                                 captureCtx->interfaceType,
                                 captureCtx->phyMode,
                                 testArgs);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to set ICP settings\n", __func__);
            goto failed;
        }
    }

    /* Create NvMediaISC object to power on cameras */
    captureCtx->iscCtx =
        NvMediaISCRootDeviceCreate(
            ISC_RDEV_CFG(captureCtx->interfaceType,
                            captureCtx->i2cDeviceNum)); /* port */
    if (!captureCtx->iscCtx) {
        LOG_ERR("%s: Failed to create NvMedia ISC root device\n", __func__);
        status = NVMEDIA_STATUS_ERROR;
        goto failed;
    }

    /* Delay for 50ms in order to let sensor power on*/
    nvsleep(50000);

    /* Write pre-requsite registers over i2c */
    status = I2cProcessInitialRegisters(&captureCtx->parsedCommands,
                                        captureCtx->i2cDeviceNum);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to write to initial registers over I2C\n", __func__);
        goto failed;
    }

    LOG_DBG("%s: Creating ICP context\n", __func__);

    /* Create NvMediaICPEx object */
    // ANIL: causes errors if I remove windowID (and I assume other fields from TestArgs)
    captureCtx->icpExCtx = NvMediaICPCreateEx(&captureCtx->icpSettingsEx);
    if (!captureCtx->icpExCtx) {
        LOG_ERR("%s: NvMediaICPCreateEx failed\n", __func__);
        status = NVMEDIA_STATUS_ERROR;
        goto failed;
    }

    /* Write registers from script file over i2c */
    status = I2cProcessCommands(&captureCtx->parsedCommands,
                                I2C_WRITE,
                                captureCtx->i2cDeviceNum);
    if (status != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to write to registers over I2C\n", __func__);
        goto failed;
    }

    /* Calibrate sensor if needed */
    if (testArgs->calibrateSensorFlag && captureCtx->sensorInfo) {
        /* Populate sensor properties */
        status = captureCtx->sensorInfo->CalibrateSensor(&captureCtx->settingsCommands,
                                                         &captureCtx->calParams,
                                                         testArgs->sensorProperties);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to CalibrateSensor, check calibration values \n", __func__);
            goto failed;
        }
        /* Apply calibration settings */
        status = I2cProcessCommands(&captureCtx->settingsCommands,
                                    I2C_WRITE,
                                    captureCtx->calParams.i2cDevice);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to process register settings\n", __func__);
            goto failed;
        }
    }

    /* Create Input Queues and set data for capture threads */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {

        captureCtx->threadCtx[i].icpExCtx = captureCtx->icpExCtx;
        captureCtx->threadCtx[i].quit = captureCtx->quit;
        captureCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        captureCtx->threadCtx[i].virtualGroupIndex = i;
        captureCtx->threadCtx[i].numFramesToCapture = (testArgs->frames.isUsed)?
                                                       testArgs->frames.uIntValue : 0;
        captureCtx->threadCtx[i].numFramesToSkip = testArgs->numFramesToSkip;
        captureCtx->threadCtx[i].numFramesToWait = testArgs->numFramesToWait;
        captureCtx->threadCtx[i].numMiniburstFrames = 1;
        captureCtx->threadCtx[i].width  = NVMEDIA_ICP_SETTINGS_HANDLER(captureCtx->icpSettingsEx, i, 0)->width;
        captureCtx->threadCtx[i].height = NVMEDIA_ICP_SETTINGS_HANDLER(captureCtx->icpSettingsEx, i, 0)->height;
        captureCtx->threadCtx[i].settings = NVMEDIA_ICP_SETTINGS_HANDLER(captureCtx->icpSettingsEx, i, 0);
        captureCtx->threadCtx[i].numBuffers = captureCtx->inputQueueSize;

        /* Create inputQueue for storing captured Images */
        status = CreateImageQueue(captureCtx->device,
                                   &captureCtx->threadCtx[i].inputQueue,
                                   captureCtx->inputQueueSize,
                                   captureCtx->threadCtx[i].width,
                                   captureCtx->threadCtx[i].height,
                                   captureCtx->threadCtx[i].surfType,
                                   captureCtx->threadCtx[i].surfAllocAttrs,
                                   captureCtx->threadCtx[i].numSurfAllocAttrs);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: capture InputQueue %d creation failed\n", __func__, i);
            goto failed;
        }

        LOG_DBG("%s: Capture Input Queue %d: %ux%u, images: %u \n",
                __func__, i, captureCtx->threadCtx[i].width,
                captureCtx->threadCtx[i].height,
                captureCtx->inputQueueSize);
    }

    return NVMEDIA_STATUS_OK;
failed:
    LOG_ERR("%s: Failed to initialize Capture\n", __func__);
    return status;
}

NvMediaStatus
CaptureFini(NvMainContext *mainCtx)
{
    NvCaptureContext *captureCtx = NULL;
    NvMediaImage *image = NULL;
    NvMediaStatus status;
    uint32_t i = 0;

    if (!mainCtx)
        return NVMEDIA_STATUS_OK;

    captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT];
    if (!captureCtx)
        return NVMEDIA_STATUS_OK;

    /* Wait for threads to exit */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {
        if (captureCtx->captureThread[i]) {
            while (!captureCtx->threadCtx[i].exitedFlag) {
                LOG_DBG("%s: Waiting for capture thread %d to quit\n",
                        __func__, i);
            }
        }
    }

    *captureCtx->quit = NVMEDIA_TRUE;

    /* Destroy threads */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {
        if (captureCtx->captureThread[i]) {
            status = NvThreadDestroy(captureCtx->captureThread[i]);
            if (status != NVMEDIA_STATUS_OK)
                LOG_ERR("%s: Failed to destroy capture thread %d\n",
                        __func__, i);
        }
    }

    /* Destroy input queues */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {
        if (captureCtx->threadCtx[i].inputQueue) {
            while ((NvQueueGet(captureCtx->threadCtx[i].inputQueue, &image,
                        0)) == NVMEDIA_STATUS_OK) {
                if (image) {
                    NvMediaImageDestroy(image);
                    image = NULL;
                }
            }
            LOG_DBG("%s: Destroying capture input queue %d \n", __func__, i);
            NvQueueDestroy(captureCtx->threadCtx[i].inputQueue);
        }
    }

    /* Destroy sensor properties */
    if(captureCtx->testArgs->sensorProperties)
        free(captureCtx->testArgs->sensorProperties);

    /* Read Sensor Registers */
    if (captureCtx->testArgs->rdregs.isUsed) {
        status = _ReadSensorRegisters(captureCtx, captureCtx->testArgs->rdregs.stringValue);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to read sensor registers\n", __func__);
        }
    }

    /* Print the deserializer registers and debug status registers*/
    if (captureCtx->testArgs->logLevel == LEVEL_DBG && captureCtx->captureParams.deserAddress.isUsed) {
        _ReadDeserRegisters(captureCtx);
    }

    /* Destroy contexts */
    if (captureCtx->icpExCtx)
        NvMediaICPDestroyEx(captureCtx->icpExCtx);

    if (captureCtx->iscCtx)
        NvMediaISCRootDeviceDestroy(captureCtx->iscCtx);

    if (captureCtx->device)
        NvMediaDeviceDestroy(captureCtx->device);

    if (captureCtx)
        free(captureCtx);

    LOG_INFO("%s: CaptureFini done\n", __func__);

    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
CaptureProc(NvMainContext *mainCtx)
{
    NvMediaStatus status;
    uint32_t i=0;

    if (!mainCtx) {
        LOG_ERR("%s: Bad parameter\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    NvCaptureContext *captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT];
    NvSaveContext *saveCtx = mainCtx->ctxs[SAVE_ELEMENT];

    /* Setting the queues */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {
        CaptureThreadCtx *threadCtx = &captureCtx->threadCtx[i];
        if (threadCtx) {
            threadCtx->outputQueue = saveCtx->threadCtx[i].inputQueue;
            saveCtx->threadCtx[i].fps = &threadCtx->fps;

            // set initial fps to reasonable value
            threadCtx->fps = 30;
        }
    }

    /* Create capture threads */
    for (i = 0; i < captureCtx->numVirtualChannels; i++) {
        CaptureThreadCtx *threadCtx = &captureCtx->threadCtx[i];
        if (threadCtx) {
            threadCtx->exitedFlag = NVMEDIA_FALSE;
            status = NvThreadCreate(&captureCtx->captureThread[i],
                                    &_CaptureThreadFunc,
                                    (void *)threadCtx,
                                    NV_THREAD_PRIORITY_NORMAL);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to create captureThread %d\n",
                        __func__, i);
                threadCtx->exitedFlag = NVMEDIA_TRUE;
                goto failed;
            }
        }
    }
failed:
    return status;
}
