#include "opencvWrapper.h"

OpencvWrapper::OpencvWrapper() {
    imgBuffer = nullptr;
}

OpencvWrapper::~OpencvWrapper() {
    if(imgBuffer) {
        delete imgBuffer;
    }
}

void OpencvWrapper::hello() {
    cv::Mat img(512, 512, CV_8UC3, cv::Scalar(0));

    cv::putText(img,
        "Hello, open CV",
        cv::Point(10, img.rows/2),
        cv::FONT_HERSHEY_DUPLEX,
        1.0,
        CV_RGB(118, 185, 0),
        2);
    cv::imshow("Hello!", img);
    cv::waitKey();
}

void OpencvWrapper::display(uint8_t *data, int width, int height, int bytesPerPixel) {
    setImgBuffer(data, width, height, bytesPerPixel);
    cv::imshow("Boson", img);
    cv::waitKey(10);
}

void OpencvWrapper::setImgBuffer(uint8_t *data, int width, int height, int bytesPerPixel) {
    if(!imgBuffer) {
        imgBuffer = new uint8_t[width * height * bytesPerPixel];
        int pixelType = CV_8UC1;
        if(bytesPerPixel == 2) {
            pixelType = CV_16UC1;
        }
        img = cv::Mat(height, width, pixelType, 
            reinterpret_cast<void *>(imgBuffer));
    }
    memcpy(imgBuffer, data, width * height * bytesPerPixel * sizeof(uint8_t));
    agc();
}

void OpencvWrapper::startRecording(int fps, std::string filename) {
    // assume that a frame as been captured (img has been initialized) before calling this
    recorder = OpencvRecorder(img, fps, filename);
}

void OpencvWrapper::stopRecording() {
    recorder.stop();
}

void OpencvWrapper::recordFrame(uint8_t *data) {
    if(!recorder.recording) {
        return;
    }

    int bytesPerPixel = 1;
    if(img.type() == CV_16UC1) {
        bytesPerPixel = 2;
    }
    setImgBuffer(data, recorder.width, recorder.height, bytesPerPixel);

    recorder.captureFrame();
}

void OpencvWrapper::agc() {
    int bytesPerPixel = 1;
    if(img.type() == CV_16UC1) {
        bytesPerPixel = 2;
    }

    cv::normalize(img, img, 0, 1 << (8 * bytesPerPixel) - 1, cv::NORM_MINMAX);
}