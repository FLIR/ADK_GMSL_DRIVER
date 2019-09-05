#include <stdio.h>

#include "capture.h"
#include "bosonCommands.h"

static uint32_t
_BosonThreadFunc(void *data) {
    BosonThreadCtx *threadCtx = (BosonThreadCtx *)data;
    // NvMediaStatus status;

    while(!(*threadCtx->quit)) {
        if(threadCtx->cmd && threadCtx->cmd[0] != '\0') {
            LOG_INFO("Got %s", threadCtx->cmd);
            sprintf(threadCtx->cmd, "");
        }
    }

    threadCtx->exitedFlag = NVMEDIA_TRUE;

    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
BosonInit(NvMainContext *mainCtx) {
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
BosonFini(NvMainContext *mainCtx) {
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
BosonProc(NvMainContext *mainCtx) {
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
