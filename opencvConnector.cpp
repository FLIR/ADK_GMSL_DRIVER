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

void Opencv_display(uint8_t *data, int width, int height) {
    initWrapper();
    opencv->display(data, width, height);
}

void Opencv_startRecording(int width, int height) {
    initWrapper();
    opencv->startRecording(width, height);
}

void Opencv_stopRecording() {
    initWrapper();
    opencv->stopRecording();
}

void Opencv_recordFrame(uint8_t *data) {
    initWrapper();
    opencv->recordFrame(data);
}

#ifdef __cplusplus
}
#endif