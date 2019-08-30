#include "display.h"
#include "capture.h"
#include "opencvConnector.h"
#include "helpers.h"


static uint32_t
_DisplayThreadFunc(void *data)
{
    DisplayThreadCtx *threadCtx = (DisplayThreadCtx *)data;
    NvMediaImage *image = NULL;
    uint8_t *imgData;
    NvMediaStatus status;
    uint32_t totalCapturedFrames = 0;
    char outputFileName[MAX_STRING_SIZE];
    char buf[MAX_STRING_SIZE] = {0};
    char *calSettings = NULL;

    NVM_SURF_FMT_DEFINE_ATTR(attr);

    while (!(*threadCtx->quit)) {
        image=NULL;
        /* Wait for captured frames */
        while (NvQueueGet(threadCtx->inputQueue, &image, DISPLAY_DEQUEUE_TIMEOUT) !=
           NVMEDIA_STATUS_OK) {
            LOG_DBG("%s: displayThread input queue %d is empty\n",
                     __func__, threadCtx->virtualGroupIndex);
            if (*threadCtx->quit)
                goto loop_done;
        }

        totalCapturedFrames++;

        if (threadCtx->displayEnabled) {

            status = NvMediaSurfaceFormatGetAttrs(threadCtx->surfType,
                                                  attr,
                                                  NVM_SURF_FMT_ATTR_MAX);
            if (status != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s:NvMediaSurfaceFormatGetAttrs failed\n", __func__);
               *threadCtx->quit = NVMEDIA_TRUE;
                goto loop_done;
            }

            if (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW) {
                if(!(imgData = malloc(image->width * image->height * 
                    threadCtx->rawBytesPerPixel * sizeof(uint8_t))))
                {
                    LOG_ERR("%s: Out of memory", __func__);
                    goto loop_done;
                }
                status = ImageToBytes(image, imgData, threadCtx->rawBytesPerPixel);
                if (status != NVMEDIA_STATUS_OK) {
                    LOG_ERR("%s: imageToBytes failed for image %d in displayThread %d\n",
                            __func__, totalCapturedFrames, threadCtx->virtualGroupIndex);
                    *threadCtx->quit = NVMEDIA_TRUE;
                    goto loop_done;
                }

                Opencv_display(imgData, image->width, image->height);

            } else {
                LOG_ERR("%s: Unsupported input image type", __func__);
            }
        }

    loop_done:
        if (image) {
            if (NvQueuePut((NvQueue *)image->tag,
                           (void *)&image,
                           0) != NVMEDIA_STATUS_OK) {
                LOG_ERR("%s: Failed to put image back in queue\n", __func__);
                *threadCtx->quit = NVMEDIA_TRUE;
            };
            image = NULL;
        }
    }
    LOG_INFO("%s: Display thread exited\n", __func__);
    threadCtx->exitedFlag = NVMEDIA_TRUE;
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
DisplayInit(NvMainContext *mainCtx)
{
    NvDisplayContext *displayCtx  = NULL;
    NvCaptureContext   *captureCtx = NULL;
    TestArgs           *testArgs = mainCtx->testArgs;
    uint32_t i = 0;
    NvMediaStatus status = NVMEDIA_STATUS_ERROR;
    NvMediaSurfAllocAttr surfAllocAttrs[8];
    uint32_t numSurfAllocAttrs;

    /* allocating display context */
    mainCtx->ctxs[DISPLAY_ELEMENT]= malloc(sizeof(NvDisplayContext));
    if (!mainCtx->ctxs[DISPLAY_ELEMENT]){
        LOG_ERR("%s: Failed to allocate memory for display context\n", __func__);
        return NVMEDIA_STATUS_OUT_OF_MEMORY;
    }

    displayCtx = mainCtx->ctxs[DISPLAY_ELEMENT];
    memset(displayCtx,0,sizeof(NvDisplayContext));
    captureCtx = mainCtx->ctxs[CAPTURE_ELEMENT];

    /* initialize context */
    displayCtx->quit      =  &mainCtx->quit;
    displayCtx->testArgs  = testArgs;
    displayCtx->numVirtualChannels = testArgs->numVirtualChannels;
    displayCtx->displayEnabled = testArgs->displayEnabled;
    displayCtx->inputQueueSize = testArgs->bufferPoolSize;
    /* Create NvMedia Device */
    displayCtx->device = NvMediaDeviceCreate();
    if (!displayCtx->device) {
        status = NVMEDIA_STATUS_ERROR;
        LOG_ERR("%s: Failed to create NvMedia device\n", __func__);
        goto failed;
    }

    /* Create display input Queues and set thread data */
    for (i = 0; i < displayCtx->numVirtualChannels; i++) {
        displayCtx->threadCtx[i].quit = displayCtx->quit;
        displayCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        displayCtx->threadCtx[i].displayEnabled = testArgs->displayEnabled;
        displayCtx->threadCtx[i].sensorInfo = testArgs->sensorInfo;
        displayCtx->threadCtx[i].calParams = &captureCtx->calParams;
        displayCtx->threadCtx[i].virtualGroupIndex = captureCtx->threadCtx[i].virtualGroupIndex;
        displayCtx->threadCtx[i].surfType = captureCtx->threadCtx[i].surfType;
        displayCtx->threadCtx[i].rawBytesPerPixel = captureCtx->threadCtx[i].rawBytesPerPixel;
        NVM_SURF_FMT_DEFINE_ATTR(attr);
        status = NvMediaSurfaceFormatGetAttrs(captureCtx->threadCtx[i].surfType,
                                              attr,
                                              NVM_SURF_FMT_ATTR_MAX);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s:NvMediaSurfaceFormatGetAttrs failed\n", __func__);
            goto failed;
        }
        displayCtx->threadCtx[i].width =  (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW )?
                                           captureCtx->threadCtx[i].width/2 : captureCtx->threadCtx[i].width;
        displayCtx->threadCtx[i].height = (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW )?
                                           captureCtx->threadCtx[i].height/2 : captureCtx->threadCtx[i].height;
        displayCtx->threadCtx[i].sensorProperties = testArgs->sensorProperties;
        if (NvQueueCreate(&displayCtx->threadCtx[i].inputQueue,
                         displayCtx->inputQueueSize,
                         sizeof(NvMediaImage *)) != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to create display inputQueue %d\n",
                    __func__, i);
            status = NVMEDIA_STATUS_ERROR;
            goto failed;
        }
        if (testArgs->displayEnabled) {
            if (attr[NVM_SURF_ATTR_SURF_TYPE].value == NVM_SURF_ATTR_SURF_TYPE_RAW ) {
                /* For RAW images, create conversion queue for converting RAW to RGB images */

                surfAllocAttrs[0].type = NVM_SURF_ATTR_WIDTH;
                surfAllocAttrs[0].value = displayCtx->threadCtx[i].width;
                surfAllocAttrs[1].type = NVM_SURF_ATTR_HEIGHT;
                surfAllocAttrs[1].value = displayCtx->threadCtx[i].height;
                surfAllocAttrs[2].type = NVM_SURF_ATTR_CPU_ACCESS;
                surfAllocAttrs[2].value = NVM_SURF_ATTR_CPU_ACCESS_UNCACHED;
                numSurfAllocAttrs = 3;

                NVM_SURF_FMT_DEFINE_ATTR(surfFormatAttrs);
                NVM_SURF_FMT_SET_ATTR_RGBA(surfFormatAttrs,RGBA,UINT,8,PL);
                status = CreateImageQueue(displayCtx->device,
                                           &displayCtx->threadCtx[i].conversionQueue,
                                           displayCtx->inputQueueSize,
                                           displayCtx->threadCtx[i].width,
                                           displayCtx->threadCtx[i].height,
                                           NvMediaSurfaceFormatGetType(surfFormatAttrs, NVM_SURF_FMT_ATTR_MAX),
                                           surfAllocAttrs,
                                           numSurfAllocAttrs);
                if (status != NVMEDIA_STATUS_OK) {
                    LOG_ERR("%s: conversionQueue creation failed\n", __func__);
                    goto failed;
                }

                LOG_DBG("%s: Dipslay Conversion Queue %d: %ux%u, images: %u \n",
                        __func__, i, displayCtx->threadCtx[i].width,
                        displayCtx->threadCtx[i].height,
                        displayCtx->inputQueueSize);
            }
        }
    }
    return NVMEDIA_STATUS_OK;
failed:
    LOG_ERR("%s: Failed to initialize Dipslay\n",__func__);
    return status;
}

NvMediaStatus
DisplayFini(NvMainContext *mainCtx)
{
    NvDisplayContext *displayCtx = NULL;
    NvMediaImage *image = NULL;
    uint32_t i;
    NvMediaStatus status = NVMEDIA_STATUS_OK;

    if (!mainCtx)
        return NVMEDIA_STATUS_OK;

    displayCtx = mainCtx->ctxs[DISPLAY_ELEMENT];
    if (!displayCtx)
        return NVMEDIA_STATUS_OK;

    /* Wait for threads to exit */
    for (i = 0; i < displayCtx->numVirtualChannels; i++) {
        if (displayCtx->displayThread[i]) {
            while (!displayCtx->threadCtx[i].exitedFlag) {
                LOG_DBG("%s: Waiting for dipslay thread %d to quit\n",
                        __func__, i);
            }
        }
    }

    *displayCtx->quit = NVMEDIA_TRUE;

    /* Destroy threads */
    for (i = 0; i < displayCtx->numVirtualChannels; i++) {
        if (displayCtx->displayThread[i]) {
            status = NvThreadDestroy(displayCtx->displayThread[i]);
            if (status != NVMEDIA_STATUS_OK)
                LOG_ERR("%s: Failed to destroy dipslay thread %d\n",
                        __func__, i);
        }
    }

    for (i = 0; i < displayCtx->numVirtualChannels; i++) {
        /*For RAW Images, destroy the conversion queue */
        if (displayCtx->threadCtx[i].conversionQueue) {
            while (IsSucceed(NvQueueGet(displayCtx->threadCtx[i].conversionQueue, &image, 0))) {
                if (image) {
                    NvMediaImageDestroy(image);
                    image = NULL;
                }
            }
            LOG_DBG("%s: Destroying conversion queue \n",__func__);
            NvQueueDestroy(displayCtx->threadCtx[i].conversionQueue);
        }

        /*Flush and destroy the input queues*/
        if (displayCtx->threadCtx[i].inputQueue) {
            LOG_DBG("%s: Flushing the dipslay input queue %d\n", __func__, i);
            while (IsSucceed(NvQueueGet(displayCtx->threadCtx[i].inputQueue, &image, 0))) {
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
            NvQueueDestroy(displayCtx->threadCtx[i].inputQueue);
        }
    }

    if (displayCtx->device)
        NvMediaDeviceDestroy(displayCtx->device);

    if (displayCtx)
        free(displayCtx);

    LOG_INFO("%s: DisplayFini done\n", __func__);
    return NVMEDIA_STATUS_OK;
}

NvMediaStatus
DisplayProc(NvMainContext *mainCtx)
{
    NvDisplayContext        *displayCtx = NULL;
    uint32_t i;
    NvMediaStatus status= NVMEDIA_STATUS_OK;

    if (!mainCtx) {
        LOG_ERR("%s: Bad parameter\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }
    displayCtx = mainCtx->ctxs[DISPLAY_ELEMENT];

    /* Create thread to display images */
    for (i = 0; i < displayCtx->numVirtualChannels; i++) {
        displayCtx->threadCtx[i].exitedFlag = NVMEDIA_FALSE;
        status = NvThreadCreate(&displayCtx->displayThread[i],
                                &_DisplayThreadFunc,
                                (void *)&displayCtx->threadCtx[i],
                                NV_THREAD_PRIORITY_NORMAL);
        if (status != NVMEDIA_STATUS_OK) {
            LOG_ERR("%s: Failed to create display thread\n",
                    __func__);
            displayCtx->threadCtx[i].exitedFlag = NVMEDIA_TRUE;
        }
    }
    return status;
}

