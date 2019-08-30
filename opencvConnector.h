#ifndef WRAPPER_CONNECTOR_H 
#define WRAPPER_CONNECTOR_H 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Opencv_hello();
void Opencv_display(uint8_t *data, int width, int height);
void Opencv_startRecording();
void Opencv_stopRecording();
void Opencv_recordFrame(uint8_t *data, int width, int height);

#ifdef __cplusplus
}
#endif


#endif