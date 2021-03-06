/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
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
            uint32_t rawBytesPerPixel,
            uint8_t multiplex);

void
MsbToLsb32(uint32_t *dest, uint8_t *src);

void
LsbToMsbArr(uint8_t *dest, uint32_t src);

int
EndsWith(const char *str, const char *suffix);

#endif