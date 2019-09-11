#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "os_common.h"

#include "capture.h"
#include "bosonCommands.h"
#include "commandListener.h"

static uint32_t
_BosonThreadFunc(void *data) {
    BosonThreadCtx *threadCtx = (BosonThreadCtx *)data;
    NvMediaStatus status = NVMEDIA_STATUS_OK;
    uint32_t response = 0;
    char responseStr[64];

    while(!(*threadCtx->quit)) {
        if(threadCtx->cmd && threadCtx->cmd[0] != '\0') {
            if(!strcasecmp(threadCtx->cmd, "f")) {
                status = TriggerShutter(0, threadCtx->sensorAddress);
            } else if(!strcasecmp(threadCtx->cmd, "sn")) {
                status = GetSerialNumber(0, threadCtx->sensorAddress, &response);
                printf("Serial number: %d\n", response);
            } else if(!strcasecmp(threadCtx->cmd, "w")) {
                status = SetColors(0, threadCtx->sensorAddress, WHITE_HOT);
            } else if(!strcasecmp(threadCtx->cmd, "b")) {
                status = SetColors(0, threadCtx->sensorAddress, BLACK_HOT);
            } else if(!strcasecmp(threadCtx->cmd, "fa")) {
                status = SetFFCMode(0, threadCtx->sensorAddress, AUTO_FFC);
            } else if(!strcasecmp(threadCtx->cmd, "fm")) {
                status = SetFFCMode(0, threadCtx->sensorAddress, MANUAL_FFC);
            } else if(!strcasecmp(threadCtx->cmd, "p")) {
                status = GetTelemetryPacking(0, threadCtx->sensorAddress, &response);
                PackingToString(response, responseStr);
                printf("Telemetry packing: %s\n", responseStr);
            } else if(!strcasecmp(threadCtx->cmd, "c")) {
                status = GetColorMode(0, threadCtx->sensorAddress, &response);
                ColorToString(response, responseStr);
                printf("Color mode: %s\n", responseStr);
            } else if(!strcasecmp(threadCtx->cmd, "pn")) {
                char pn[32];
                status = GetPartNumber(0, threadCtx->sensorAddress, pn);
                printf("Part number: %s\n", pn);
            }  else {
                LOG_INFO("%s: Unsupported input", __func__);
            }
            if(status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Unable to send I2C command", __func__);
            }

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
