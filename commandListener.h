#ifndef __COMMAND_LISTENER_H__
#define __COMMAND_LISTENER_H__

#include <stdio.h>
#include "nvmedia_core.h"
#include "thread_utils.h"

#include "main.h"

typedef struct {
    uint32_t                sensorAddress;
    char                   *cmd;
    volatile NvMediaBool   *quit;
    volatile NvMediaBool   *toggleRecording;
    NvMediaBool             exitedFlag;
} BosonThreadCtx;

typedef struct {
    NvThread               *bosonThread[NVMEDIA_ICP_MAX_VIRTUAL_GROUPS];
    BosonThreadCtx          threadCtx[NVMEDIA_ICP_MAX_VIRTUAL_GROUPS];
    uint32_t                numVirtualChannels;
    volatile NvMediaBool   *quit;
    char                   *cmd;
} NvBosonContext;

NvMediaStatus
ListenerInit(NvMainContext *mainCtx);

NvMediaStatus
ListenerFini(NvMainContext *mainCtx);

NvMediaStatus
ListenerProc(NvMainContext *mainCtx);

#endif