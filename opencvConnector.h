#ifndef WRAPPER_CONNECTOR_H 
#define WRAPPER_CONNECTOR_H 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Opencv_hello();
void Opencv_sendFrame(uint8_t *data, int width, int height, int bytesPerPixel);
void Opencv_sendTelemetry(uint8_t *data, int stride);
void Opencv_display();
void Opencv_startRecording(int fps, char *filename);
void Opencv_stopRecording();
void Opencv_recordFrame();
uint32_t Opencv_getSerialNumber();
void Opencv_getFrame(uint8_t *data);
void Opencv_getTelemetry(uint8_t *telemetry);
void Opencv_captureImage(char *filename);

#ifdef __cplusplus
}
#endif


#endif