#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#include <stdint.h>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc.hpp"
#include <opencv2/imgcodecs.hpp>
#include "opencv2/videoio.hpp"
#include "opencv2/opencv.hpp"

class OpencvWrapper {
    public:
        OpencvWrapper();
        ~OpencvWrapper();
        void hello();
        void display(uint8_t *data, int width, int height);
        void startRecording(int width, int height);
        void stopRecording();
        void recordFrame(uint8_t *data);
    private:
        uint8_t *imgBuffer;
        bool recording;
        int videoWidth;
        int videoHeight;
        cv::Mat img;
        cv::VideoWriter videoRecorder;

        void setImgBuffer(uint8_t *data, int width, int height);
};

#endif