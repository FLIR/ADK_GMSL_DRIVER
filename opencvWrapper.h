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
        void display(uint8_t *data, int width, int height, int bytesPerPixel);
        void startRecording(int fps, std::string filename);
        void stopRecording();
        void recordFrame(uint8_t *data);
    private:
        uint8_t *imgBuffer;
        cv::Mat img;
        OpencvRecorder recorder;

        void setImgBuffer(uint8_t *data, int width, int height, int bytesPerPixel);
};

#endif