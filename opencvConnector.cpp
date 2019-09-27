#include <cstdlib>

#include "opencvConnector.h"
#include "opencvWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "log_utils.h"

// Inside this "extern C" block, I can define C functions that are able to call C++ code

static OpencvWrapper *opencv = NULL;

void initWrapper(int width, int height, int bytesPerPixel) {
    if (opencv == NULL) {
        opencv = new OpencvWrapper(width, height, bytesPerPixel);
    }
}

void Opencv_hello() {
    initWrapper(512, 512, 1);
    opencv->hello();
}

void Opencv_sendFrame(uint8_t *data, int width, int height, int bytesPerPixel) {
    initWrapper(width, height, bytesPerPixel);
    opencv->sendFrame(data);
}

void Opencv_sendTelemetry(uint8_t *data, int stride) {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }
    opencv->sendTelemetry(data, stride);
}

void Opencv_display() {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }
    opencv->display();
}

void Opencv_startRecording(int fps, char *filename) {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }
    opencv->startRecording(fps, filename);
}

void Opencv_stopRecording() {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }
    opencv->stopRecording();
}

void Opencv_recordFrame() {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }
    opencv->recordFrame();
}

uint32_t Opencv_getSerialNumber() {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return 0;
    }
    return opencv->getSerialNumber();
}

void Opencv_getFrame(uint8_t *data) {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }

    return opencv->getFrame(data);
}

void Opencv_getTelemetry(uint8_t *telemetry) {
    if(!opencv) {
        LOG_ERR("OpenCV object must be initialized");
        return;
    }

    return opencv->getTelemetry(telemetry);
}

#ifdef __cplusplus
}
#endif