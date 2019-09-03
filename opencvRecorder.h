#ifndef __OPENCV_RECORDER_H__
#define __OPENCV_RECORDER_H__

#include <stdlib.h>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc.hpp"
#include <opencv2/imgcodecs.hpp>
#include "opencv2/videoio.hpp"

class OpencvRecorder {
    public:
        int width, height, bytesPerPixel;
        bool recording;

        OpencvRecorder();
        OpencvRecorder(cv::Mat img, int fps, std::string filename);
        ~OpencvRecorder();
        void captureFrame();
        void stop();
    private:
        cv::VideoWriter recorder;
        cv::Mat image;
};

#endif