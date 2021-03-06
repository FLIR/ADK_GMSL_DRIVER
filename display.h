/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "thread_utils.h"

#include "main.h"
#include "cmdline.h"

#define DISPLAY_QUEUE_SIZE                 3      /* min no. of buffers to be in circulation at any point */
#define DISPLAY_DEQUEUE_TIMEOUT            1000
#define DISPLAY_ENQUEUE_TIMEOUT            100

typedef struct {
    NvQueue                    *inputQueue;
    NvQueue                    *outputQueue;
    volatile NvMediaBool       *quit;
    NvMediaBool                 displayEnabled;
    NvMediaBool                 exitedFlag;

    /* display params */
    uint32_t                    rawBytesPerPixel;
    uint32_t                    virtualGroupIndex;

    /* Raw2Rgb conversion params */
    NvMediaSurfaceType          surfType;
    uint32_t                    width;
    uint32_t                    height;
} DisplayThreadCtx;

typedef struct {
    /* 2D processing */
    NvThread                   *displayThread[NVMEDIA_ICP_MAX_VIRTUAL_GROUPS];
    DisplayThreadCtx            threadCtx[NVMEDIA_ICP_MAX_VIRTUAL_GROUPS];
    NvMediaDevice              *device;

    /* General processing params */
    volatile NvMediaBool       *quit;
    TestArgs                   *testArgs;
    NvMediaBool                 displayEnabled;
    uint32_t                    numVirtualChannels;
    uint32_t                    inputQueueSize;
} NvDisplayContext;

NvMediaStatus
DisplayInit(NvMainContext *mainCtx);

NvMediaStatus
DisplayFini(NvMainContext *mainCtx);

NvMediaStatus
DisplayProc(NvMainContext *mainCtx);


#endif