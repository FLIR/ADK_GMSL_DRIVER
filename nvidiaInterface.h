#ifndef NVIDIA_INTERFACE_H
#define NVIDIA_INTERFACE_H

#include <iostream>
#include <cstdint>

extern "C" {
    #include "main.h"
    #include "cmdline.h"    
    #include "bosonCommands.h"
}

namespace BosonAPI {

typedef struct {
    std::string regFile;
    int displayId;
    int logLevel;
} CmdArgs;

class NvidiaInterface {
    public:
        NvidiaInterface();
        ~NvidiaInterface();
        void run(CmdArgs args);
        void runC(TestArgs *args);
        bool isRunning();
        std::string getUserInput();
        void flushInput();
        void getFrame(void *frame);
        void startRecording(std::string filename);
        void stopRecording();
        void ffc();
        void toggleHeater();
        uint32_t getSerialNumber();
        void setColors(BosonColor color);
        std::string getSceneColor();
        void setFfcMode(FFCMode mode);
        std::string getFfcMode();
        std::string getPartNumber();
        uint32_t getFps();
        std::string getVideoType();
        void runI2CCommand(uint32_t cmd);
        uint32_t getI2CInt(uint32_t cmd);
        std::string getI2CString(uint32_t cmd);
        void setI2CInt(uint32_t cmd, uint32_t val);
    private:
        int i2cDevice = -1;
        int sensorAddress = -1;

        NvMainContext mainCtx;
        bool getI2CInfo(char *filename, int *deviceHandle, int *sensorHandle);
};

}

#endif