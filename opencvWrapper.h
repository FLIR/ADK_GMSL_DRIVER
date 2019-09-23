#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#include <stdint.h>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc.hpp"

#include "opencvRecorder.h"

class OpencvWrapper {
    public:
        OpencvWrapper(int width, int height, int bytesPerPixel);
        ~OpencvWrapper();
        void hello();
        void sendFrame(uint8_t *data);
        void getFrame(uint8_t *data);
        void display();
        void startRecording(int fps, std::string filename);
        void stopRecording();
        void recordFrame();
        void sendTelemetry(uint8_t *data, int stride);
        uint32_t getSerialNumber();
    private:
        int width, height, bytesPerPixel;
        uint8_t *imgBuffer;
        cv::Mat img;
        OpencvRecorder recorder;
        uint32_t serialNumber;

        void setImgBuffer(uint8_t *data);
        void agc();
};

#endif