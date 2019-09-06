#include <cstdlib>

#include "opencvConnector.h"
#include "opencvWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inside this "extern C" block, I can define C functions that are able to call C++ code

static OpencvWrapper *opencv = NULL;

void initWrapper() {
    if (opencv == NULL) {
        opencv = new OpencvWrapper();
    }
}

void Opencv_hello() {
    initWrapper();
    opencv->hello();
}

void Opencv_sendFrame(uint8_t *data, int width, int height, int bytesPerPixel) {
    initWrapper();
    opencv->sendFrame(data, width, height, bytesPerPixel);
}

void Opencv_sendTelemetry(uint8_t *data, int stride) {
    initWrapper();
    opencv->sendTelemetry(data, stride);
}

void Opencv_display() {
    initWrapper();
    opencv->display();
}

void Opencv_startRecording(int fps, char *filename) {
    initWrapper();
    opencv->startRecording(fps, filename);
}

void Opencv_stopRecording() {
    initWrapper();
    opencv->stopRecording();
}

void Opencv_recordFrame() {
    initWrapper();
    opencv->recordFrame();
}

uint32_t Opencv_getSerialNumber() {
    initWrapper();
    return opencv->getSerialNumber();
}

#ifdef __cplusplus
}
#endif