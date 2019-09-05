#ifndef __BOSON_COMMANDS_H__
#define __BOSON_COMMANDS_H__

#include <stdio.h>
#include "nvmedia_core.h"
#include "thread_utils.h"

#include "main.h"

typedef struct {
    uint32_t                sensorAddress;
    char                   *cmd;
    volatile NvMediaBool   *quit;
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
BosonInit(NvMainContext *mainCtx);

NvMediaStatus
BosonFini(NvMainContext *mainCtx);

NvMediaStatus
BosonProc(NvMainContext *mainCtx);

#endif