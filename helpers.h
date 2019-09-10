#ifndef __HELPERS_H__
#define __HELPERS_H__

#include "nvmedia_core.h"
#include "nvmedia_image.h"
#include "thread_utils.h"

NvMediaStatus 
CreateImageQueue(NvMediaDevice *device,
                NvQueue **queue,
                uint32_t queueSize,
                uint32_t width,
                uint32_t height,
                NvMediaSurfaceType surfType,
                NvMediaSurfAllocAttr *surfAllocAttrs,
                uint32_t numSurfAllocAttrs);

NvMediaStatus
ImageToBytes(NvMediaImage *imgSrc,
            uint8_t *dstBuffer,
            uint8_t *telemetry,
            uint32_t rawBytesPerPixel);

void
LsbToMsb32(uint32_t *dest, uint8_t *src);

#endif