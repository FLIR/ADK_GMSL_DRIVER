/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
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
        int width, height;
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