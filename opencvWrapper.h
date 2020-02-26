/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
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
        // hello world display for testing openCV operability
        void hello();
        // saves new frame to object state
        void sendFrame(uint8_t *data);
        // saves telemetry data to object state
        void sendTelemetry(uint8_t *data, int stride);
        // returns a copy of the current frame data
        void getFrame(uint8_t *data);
        // returns a copy of the telemetry data
        void getTelemetry(uint8_t *data);
        // sends frame to openCV display window
        void display();
        // starts recording video
        void startRecording(int fps, std::string filename);
        // stops recording video
        void stopRecording();
        // writes frame to video
        void recordFrame();
        // saves still image
        void saveImage(std::string filename);
        // gets serial number from telemetry data
        uint32_t getSerialNumber();
    private:
        int width, height, bytesPerPixel;
        uint8_t *imgBuffer;
        uint8_t *telemetry;
        cv::Mat img;
        OpencvRecorder recorder;
        uint32_t serialNumber;

        void setImgBuffer(uint8_t *data);
        void agc();
};

#endif