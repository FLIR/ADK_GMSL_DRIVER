#include <stdio.h>
#include <string.h>

#include "log_utils.h"
#include "misc_utils.h"

#include "helpers.h"

NvMediaStatus
CreateImageQueue(NvMediaDevice *device,
                  NvQueue **queue,
                  uint32_t queueSize,
                  uint32_t width,
                  uint32_t height,
                  NvMediaSurfaceType surfType,
                  NvMediaSurfAllocAttr *surfAllocAttrs,
                  uint32_t numSurfAllocAttrs)
{
    uint32_t j = 0;
    NvMediaImage *image = NULL;
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    if (NvQueueCreate(queue,
                      queueSize,
                      sizeof(NvMediaImage *)) != NVMEDIA_STATUS_OK) {
       LOG_ERR("%s: Failed to create image Queue \n", __func__);
       return NVMEDIA_STATUS_OK;
    }

    for (j = 0; j < queueSize; j++) {
        LOG_DBG("%s: NvMediaImageCreateNew\n", __func__);
        image =  NvMediaImageCreateNew(device,           // device
                                    surfType,           // NvMediaSurfaceType type
                                    surfAllocAttrs,     // surf allocation attrs
                                    numSurfAllocAttrs,  // num attrs
                                    0);                 // flags
        if (!image) {
            LOG_ERR("%s: NvMediaImageCreate failed for image %d",
                        __func__, j);
            return NVMEDIA_STATUS_ERROR;
        }

        image->tag = *queue;

        if (IsFailed(NvQueuePut(*queue,
                                (void *)&image,
                                NV_TIMEOUT_INFINITE))) {
            LOG_ERR("%s: Pushing image to image queue failed\n", __func__);
            return NVMEDIA_STATUS_ERROR;
        }
    }

    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
ImageToBytes(NvMediaImage *imgSrc,
              uint8_t *dstBuffer,
              uint8_t *telemetry,
              uint32_t rawBytesPerPixel)
{
    uint8_t *pSrcBuff = NULL;
    NvMediaImageSurfaceMap surfaceMap;
    NvMediaStatus status;

    uint32_t srcWidth, srcHeight, srcPitch; 

    if (NvMediaImageLock(imgSrc, NVMEDIA_IMAGE_ACCESS_WRITE, &surfaceMap) !=
        NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: NvMediaImageLock failed\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    srcWidth = surfaceMap.width;
    srcHeight = surfaceMap.height;
    srcPitch = srcWidth * rawBytesPerPixel;

    if (!(pSrcBuff = malloc(srcPitch * srcHeight * sizeof(uint8_t)))) {
        LOG_ERR("%s: Out of memory\n", __func__);
        return NVMEDIA_STATUS_OUT_OF_MEMORY;
    }

    status = NvMediaImageGetBits(imgSrc, NULL, (void **)&pSrcBuff, &srcPitch);
    NvMediaImageUnlock(imgSrc);

    // skip the first row (telemetry line)
    memcpy(dstBuffer, &pSrcBuff[srcPitch], 
        srcPitch * (srcHeight - 1) * sizeof(uint8_t));

    // get telemetry data
    memcpy(telemetry, pSrcBuff, srcPitch * sizeof(uint8_t));

    // FILE *fp = fopen("img.out", "w");
    // fwrite(dstBuffer, srcPitch * (srcHeight - 1), sizeof(uint8_t), fp);
    // fclose(fp);    

    if(pSrcBuff)
        free(pSrcBuff);

    return NVMEDIA_STATUS_OK;
}

void
MsbToLsb32(uint32_t *dest, uint8_t *src) {
    *dest = 0;
    for (size_t i = 0; i < 4; i++) {
        *dest += src[i] << (24 - (8 * i));
    }
}

void
LsbToMsbArr(uint8_t *dest, uint32_t src) {
    for (size_t i = 0; i < 4; i++) {
        dest[i] = (src >> (24 - (8 * i))) & 0xFF;
    }
}

void
SplitString(char **dest, char *src, char *delim) {
    char *pch;
    uint32_t i = 0;

    pch = strtok(src, delim);
    while(pch != NULL) {
        dest[i] = pch;
        pch = strtok(NULL, delim);
        i++;
    }
}