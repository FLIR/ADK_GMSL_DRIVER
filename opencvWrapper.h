#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#include <stdint.h>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc.hpp"

class OpencvWrapper {
    public:
        OpencvWrapper();
        ~OpencvWrapper();
        void hello();
        void display(uint8_t *data, int width, int height);
    private:
        uint8_t *imgBuffer;
        cv::Mat img;
};

#endif