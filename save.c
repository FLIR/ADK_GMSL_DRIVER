/* Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <limits.h>
#include <math.h>

#include "capture.h"
#include "save.h"
#include "display.h"
#include "opencvConnector.h"
#include "helpers.h"

static void
_CreateOutputFileName(char *saveFilePrefix,
                      char *calSettings,
                      uint32_t virtualGroupIndex,
                      uint32_t frame,
                      NvMediaBool useNvRawFormat,
                      char *outputFileName)
{
    char buf[MAX_STRING_SIZE] = {0};

    memset(outputFileName, 0, MAX_STRING_SIZE);
    strncpy(outputFileName, saveFilePrefix, MAX_STRING_SIZE);
    if(calSettings)
        strcat(outputFileName, calSettings);
    strcat(outputFileName, "_vc");
    sprintf(buf, "%d", virtualGroupIndex);
    strcat(outputFileName, buf);
    strcat(outputFileName, "_");
    sprintf(buf, "%02d", frame);
    strcat(outputFileName, buf);
    if (useNvRawFormat)
        strcat(outputFileName, ".nvraw");
    else
        strcat(outputFileName, ".raw");
}

static uint32_t
_SaveThreadFunc(void *data)
{
    SaveThreadCtx *threadCtx = (SaveThreadCtx *)data;
    NvMediaImage *image = NULL;
    uint8_t *imgData;
    NvMediaStatus status;
    uint32_t totalSavedFrames=0;

    char outputFileName[MAX_STRING_SIZE];
    char buf[MAX_STRING_SIZE] = {0};
    char *calSettings = NULL;

    NVM_SURF_FMT_DEFINE_ATTR(attr);

    while (!(*threadCtx->quit)) {
        image=NULL;
        /* Wait for captured frames */
        while (NvQueueGet(threadCtx->inputQueue, &image, SAVE_DEQUEUE_TIMEOUT) !=
           NVMEDIA_STATUS_OK) {
            LOG_DBG("%s: saveThread input queue %d is empty\n",
                     __func__, threadCtx->virtualGroupIndex);
            if (*threadCtx->quit)
                goto loop_done;
        }

        if (*threadCtx->toggleRecording) {
            threadCtx->videoEnabled = (NvMediaBool)(!threadCtx->videoEnabled);
            if(threadCtx->videoEnabled) {
                Opencv_startRecording(image->width, image->height);
            } else {
                Opencv_stopRecording();
            }
            *threadCtx->toggleRecording = NVMEDIA_FALSE;
        }

        if(threadCtx->videoEnabled) {
            if(!(imgData = malloc(image->width * image->height * sizeof(uint8_t)))) {
                LOG_ERR("%s: Out of memory", __func__);
                goto loop_done;
            }

            status = ImageToBytes(image, imgData, threadCtx->rawBytesPerPixel);
            if(status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Could not convert image to bytes", __func__);
                goto loop_done;
            }

            Opencv_recordFrame(imgData);
        }

    loop_done:
        if (image) {
            if (NvQueuePut(threadCtx->outputQueue,
                           (void *)&image,
                           0) != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to put image back in queue\n", __func__);
                *threadCtx->quit = NVMEDIA_TRUE;
            };
        }
    }
    LOG_INFO("%s: Save thread exited\n", __func__);
    threadCtx->exitedFlag = NVMEDIA_TRUE;
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
SaveInit(NvMainContext *mainCtx)
{
    NvSaveContext *saveCtx  = NULL;
    NvCaptureContext   *captureCtx = NULL;
    TestArgs           *testArgs = mainCtx->testArgs;
    uint32_t i = 0;
    NvMediaStatus status = NVMEDIA_STATUS_ERROR;
    NvMediaSurfAllocAttr surfAllocAttrs[8];
    uint32_t numSurfAllocAttrs;

    /* allocating save context */
    mainCtx->ctxs[SAVE_ELEMENT]= malloc(sizeof(NvSaveContext));
    if (!mainCtx->ctxs[SAVE_ELEMENT]){
        LOG_ERR("%s: Failed to allocate memory for save context\n", __func__);
        return NVMEDIA_STATUS_OUT_OF_MEMORY;
    }

    saveCtx = mainCtx->ctxs[SAVE_ELEMENT];
    memset(saveCtx,0,sizeof(NvSaveContext));
    captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT];

    /* initialize context */
    saveCtx->quit      =  &mainCtx->quit;
    saveCtx->testArgs  = testArgs;
    saveCtx->numVirtualChannels = testArgs->numVirtualChannels;
    saveCtx->displayEnabled = testArgs->displayEnabled;
    saveCtx->inputQueueSize = testArgs->bufferPoolSize;
    /* Create NvMedia Device */
    saveCtx->device = NvMediaDeviceCreate();
    if (!saveCtx->device) {
        status = NVMEDIA_STATUS_ERROR;
        LOG_ERR("%s: Failed to create NvMedia device\n", __func__);
        goto failed;
    }

    /* Create save input Queues and set thread data */
    for (i = 0; i < saveCtx->numVirtualChannels; i++) {
        saveCtx->threadCtx[i].quit = saveCtx->quit;
        saveCtx->threadCtx[i].toggleRecording = &mainCtx->toggleRecording;
        saveCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        saveCtx->threadCtx[i].saveFilePrefix = testArgs->filePrefix;
        saveCtx->threadCtx[i].sensorInfo = testArgs->sensorInfo;
        saveCtx->threadCtx[i].calParams = &captureCtx->calParams;
        saveCtx->threadCtx[i].virtualGroupIndex = captureCtx->threadCtx[i].virtualGroupIndex;
        saveCtx->threadCtx[i].numFramesToSave = (testArgs->frames.isUsed)?
                                                 testArgs->frames.uIntValue : 0;
        saveCtx->threadCtx[i].pixelOrder = captureCtx->threadCtx[i].pixelOrder;
        saveCtx->threadCtx[i].rawBytesPerPixel = captureCtx->threadCtx[i].rawBytesPerPixel;
        NVM_SURF_FMT_DEFINE_ATTR(attr);
        status = NvMediaSurfaceFormatGetAttrs(captureCtx->threadCtx[i].surfType,
                                              attr,
                                              NVM_SURF_FMT_ATTR_MAX);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s:NvMediaSurfaceFormatGetAttrs failed\n", __func__);
            goto failed;
        }
        saveCtx->threadCtx[i].width =  (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW )?
                                           captureCtx->threadCtx[i].width/2 : captureCtx->threadCtx[i].width;
        saveCtx->threadCtx[i].height = (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW )?
                                           captureCtx->threadCtx[i].height/2 : captureCtx->threadCtx[i].height;
        saveCtx->threadCtx[i].sensorProperties = testArgs->sensorProperties;
        if (NvQueueCreate(&saveCtx->threadCtx[i].inputQueue,
                         saveCtx->inputQueueSize,
                         sizeof(NvMediaImage *)) != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to create save inputQueue %d\n",
                    __func__, i);
            status = NVMEDIA_STATUS_ERROR;
            goto failed;
        }
        if (testArgs->displayEnabled) {
            if (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW ) {
                /* For RAW images, create conversion queue for converting RAW to RGB images */

                surfAllocAttrs[0].type = NVM_SURF_ATTR_WIDTH;
                surfAllocAttrs[0].value = saveCtx->threadCtx[i].width;
                surfAllocAttrs[1].type = NVM_SURF_ATTR_HEIGHT;
                surfAllocAttrs[1].value = saveCtx->threadCtx[i].height;
                surfAllocAttrs[2].type = NVM_SURF_ATTR_CPU_ACCESS;
                surfAllocAttrs[2].value = NVM_SURF_ATTR_CPU_ACCESS_UNCACHED;
                numSurfAllocAttrs = 3;

                NVM_SURF_FMT_DEFINE_ATTR(surfFormatAttrs);
                NVM_SURF_FMT_SET_ATTR_RGBA(surfFormatAttrs,RGBA,UINT,8,PL);
                status = CreateImageQueue(saveCtx->device,
                                           &saveCtx->threadCtx[i].conversionQueue,
                                           saveCtx->inputQueueSize,
                                           saveCtx->threadCtx[i].width,
                                           saveCtx->threadCtx[i].height,
                                           NvMediaSurfaceFormatGetType(surfFormatAttrs, NVM_SURF_FMT_ATTR_MAX),
                                           surfAllocAttrs,
                                           numSurfAllocAttrs);
                if (status != NVMEDIA_STATUS_OK) {
                    LOG_ERR("%s: conversionQueue creation failed\n", __func__);
                    goto failed;
                }

                LOG_DBG("%s: Save Conversion Queue %d: %ux%u, images: %u \n",
                        __func__, i, saveCtx->threadCtx[i].width,
                        saveCtx->threadCtx[i].height,
                        saveCtx->inputQueueSize);
            }
        }
    }
    return NVMEDIA_STATUS_OK;
failed:
    LOG_ERR("%s: Failed to initialize Save\n",__func__);
    return status;
}

NvMediaStatus
SaveFini(NvMainContext *mainCtx)
{
    NvSaveContext *saveCtx = NULL;
    NvMediaImage *image = NULL;
    uint32_t i;
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    if (!mainCtx)
        return NVMEDIA_STATUS_OK;

    saveCtx = mainCtx->ctxs[SAVE_ELEMENT];
    if (!saveCtx)
        return NVMEDIA_STATUS_OK;

    /* Wait for threads to exit */
    for (i = 0; i < saveCtx->numVirtualChannels; i++) {
        if (saveCtx->saveThread[i]) {
            while (!saveCtx->threadCtx[i].exitedFlag) {
                LOG_DBG("%s: Waiting for save thread %d to quit\n",
                        __func__, i);
            }
        }
    }

    *saveCtx->quit = NVMEDIA_TRUE;

    /* Destroy threads */
    for (i = 0; i < saveCtx->numVirtualChannels; i++) {
        if (saveCtx->saveThread[i]) {
            status = NvThreadDestroy(saveCtx->saveThread[i]);
            if (status != NVMEDIA_STATUS_OK)
                LOG_ERR("%s: Failed to destroy save thread %d\n",
                        __func__, i);
        }
    }

    for (i = 0; i < saveCtx->numVirtualChannels; i++) {
        /*For RAW Images, destroy the conversion queue */
        if (saveCtx->threadCtx[i].conversionQueue) {
            while (IsSucceed(NvQueueGet(saveCtx->threadCtx[i].conversionQueue, &image, 0))) {
                if (image) {
                    NvMediaImageDestroy(image);
                    image = NULL;
                }
            }
            LOG_DBG("%s: Destroying conversion queue \n",__func__);
            NvQueueDestroy(saveCtx->threadCtx[i].conversionQueue);
        }

        /*Flush and destroy the input queues*/
        if (saveCtx->threadCtx[i].inputQueue) {
            LOG_DBG("%s: Flushing the save input queue %d\n", __func__, i);
            while (IsSucceed(NvQueueGet(saveCtx->threadCtx[i].inputQueue, &image, 0))) {
                if (image) {
                    if (NvQueuePut((NvQueue *)image->tag,
                                   (void *)&image,
                                   0) != NVMEDIA_STATUS_OK) {
                        LOG_ERR("%s: Failed to put image back in queue\n", __func__);
                        break;
                    }
                }
                image=NULL;
            }
            NvQueueDestroy(saveCtx->threadCtx[i].inputQueue);
        }
    }

    if (saveCtx->device)
        NvMediaDeviceDestroy(saveCtx->device);

    if (saveCtx)
        free(saveCtx);

    LOG_INFO("%s: SaveFini done\n", __func__);
    return NVMEDIA_STATUS_OK;
}


NvMediaStatus
SaveProc(NvMainContext *mainCtx)
{
    NvSaveContext        *saveCtx = NULL;
    NvDisplayContext     *displayCtx = NULL;
    uint32_t i;
    NvMediaStatus status= NVMEDIA_STATUS_OK;

    if (!mainCtx) {
        LOG_ERR("%s: Bad parameter\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }
    saveCtx = mainCtx->ctxs[SAVE_ELEMENT];
    displayCtx = mainCtx->ctxs[DISPLAY_ELEMENT];

    /* Setting the queues */
    if (saveCtx->displayEnabled) {
        for (i = 0; i < saveCtx->numVirtualChannels; i++) {
            saveCtx->threadCtx[i].outputQueue = &displayCtx->threadCtx->inputQueue[i];
        }
    }

    /* Create thread to save images */
    for (i = 0; i < saveCtx->numVirtualChannels; i++) {
        saveCtx->threadCtx[i].exitedFlag = NVMEDIA_FALSE;
        status = NvThreadCreate(&saveCtx->saveThread[i],
                                &_SaveThreadFunc,
                                (void *)&saveCtx->threadCtx[i],
                                NV_THREAD_PRIORITY_NORMAL);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to create save Thread\n",
                    __func__);
            saveCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        }
    }
    return status;
}
