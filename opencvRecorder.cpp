#include "opencvRecorder.h"

OpencvRecorder::OpencvRecorder() {
    recording = false;
}

OpencvRecorder::OpencvRecorder(cv::Mat img, int fps, std::string filename) {
    image = img;
    width = img.cols;
    height = img.rows;

    // there does not seem to be a codec for saving 16 bit grayscale video
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