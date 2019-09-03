#include "opencvRecorder.h"

OpencvRecorder::OpencvRecorder() {
    recording = false;
}

OpencvRecorder::OpencvRecorder(cv::Mat img, int fps, std::string filename) {
    image = img;
    width = img.cols;
    height = img.rows;

    recorder = cv::VideoWriter(filename, CV_8UC1, fps, cv::Size(width, height), false);
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