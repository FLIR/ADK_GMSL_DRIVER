#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#include <stdint.h>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc.hpp"

#include "opencvRecorder.h"

class OpencvWrapper {
    public:
        OpencvWrapper();
        ~OpencvWrapper();
        void hello();
        void sendFrame(uint8_t *data, int width, int height, int bytesPerPixel);
        void display();
        void startRecording(int fps, std::string filename);
        void stopRecording();
        void recordFrame();
    private:
        uint8_t *imgBuffer;
        cv::Mat img;
        OpencvRecorder recorder;

        void setImgBuffer(uint8_t *data, int width, int height, int bytesPerPixel);
        void agc();
};

#endif