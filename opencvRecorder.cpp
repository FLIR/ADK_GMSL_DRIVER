#include "opencvRecorder.h"

OpencvRecorder::OpencvRecorder() {
    recording = false;
}

OpencvRecorder::OpencvRecorder(cv::Mat img, int fps, std::string filename) {
    image = img;
    width = img.cols;
    height = img.rows;

    if(image.type() == CV_8UC1) {
        bytesPerPixel = 1;
    }
    if(image.type() == CV_8UC2) {
        bytesPerPixel = 2;
    }

    recorder = cv::VideoWriter(filename, image.type(), 
        fps, cv::Size(width, height), false);
    recording = true;
}

OpencvRecorder::~OpencvRecorder() {
    stop();
}

void OpencvRecorder::captureFrame() {
    recorder.write(image);
}

void OpencvRecorder::stop() {
    recording = false;
    recorder.release();
}