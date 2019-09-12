#ifndef NVIDIA_INTERFACE_H
#define NVIDIA_INTERFACE_H

#include <iostream>

typedef enum {
    WHITE_HOT = 0,
    BLACK_HOT
} PaletteColor;

typedef enum {
    MANUAL_FFC = 0,
    AUTO_FFC
} FFCMode;

typedef enum {
    PACKING_DEFAULT = 0,
    PACKING_Y,
    PACKING_8_BIT,
    PACKING_END
} TelemetryPacking;

typedef enum {
    VIDEO_MONO16 = 0,
    VIDEO_MONO8,
    VIDEO_COLOR,
    VIDEO_ANALOG,
    VIDEO_END
} VideoType;

class NvidiaInterface {
    public:
        NvidiaInterface();
        ~NvidiaInterface();
        void run(int argc, char **argv);
        void ffc();
        uint32_t getSerialNumber();
        void setColors(PaletteColor color);
        PaletteColor getSceneColor();
        void setFfcMode(FFCMode mode);
        FFCMode getFfcMode();
        std::string getPartNumber();
    private:
        bool isRunning = false;
};

#endif