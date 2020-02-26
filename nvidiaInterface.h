/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
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

typedef enum FLIR_COLOR {
    COLOR_DEFAULT = (int32_t) 0,
    COLOR_WHITEHOT = (int32_t) 0,
    COLOR_BLACKHOT = (int32_t) 1,
    COLOR_RAINBOW = (int32_t) 2,
    COLOR_RAINBOW_HC = (int32_t) 3,
    COLOR_IRONBOW = (int32_t) 4,
    COLOR_LAVA = (int32_t) 5,
    COLOR_ARCTIC = (int32_t) 6,
    COLOR_GLOBOW = (int32_t) 7,
    COLOR_GRADEDFIRE = (int32_t) 8,
    COLOR_HOTTEST = (int32_t) 9,
    COLOR_ID_END = (int32_t) 10,
};

typedef enum FLIR_FFCMODE {
    MANUAL_FFC = (int32_t) 0,
    AUTO_FFC = (int32_t) 1,
    EXTERNAL_FFC = (int32_t) 2,
    SHUTTER_TEST_FFC = (int32_t) 3,
    FFCMODE_END = (int32_t) 4,
};

typedef enum FLIR_VIDEO {
    VIDEO_MONO16 = (int32_t) 0,
    VIDEO_MONO8 = (int32_t) 1,
    VIDEO_COLOR = (int32_t) 2,
    VIDEO_ANALOG = (int32_t) 3,
    VIDEO_END = (int32_t) 4,
};


typedef struct {
    std::string regFile;
    int displayId;
    int logLevel;
} CmdArgs;


class NvidiaInterface {
    public:
        NvidiaInterface();
        ~NvidiaInterface();
        // starts streaming frames to OpenCV window
        void run(CmdArgs args);
        // starts streaming frames to OpenCV window
        void run(TestArgs *args);
        // checks whether application is running
        bool isRunning();
        // gets user input from the terminal
        std::string getUserInput();
        // clears input from the terminal
        void flushInput();
        // gets the current streaming frame pixel data
        void getFrame(uint8_t *frame);
        // gets the telemetry line
        void getTelemetry(uint8_t *telemetry);
        // starts recording video and saves stream to filename
        void startRecording(std::string filename);
        // stops recording video
        void stopRecording();
        // triggers FFC shutter
        void ffc();
        // turns on/off heater
        void toggleHeater();
        // gets camera serial number
        uint32_t getSerialNumber();
        // sets the video color (does nothing for now)
        void setColors(const FLIR_COLOR color);
        // gets the selected video color (does not reflect display)
        std::string getSceneColor();
        // sets the FFC mode to auto or manual
        void setFfcMode(FLIR_FFCMODE ffcMode);
        // gets the FFC mode
        std::string getFfcMode();
        // gets the camera part number
        std::string getPartNumber();
        // gets the streaming frames per second
        uint32_t getFps();
        // gets the video type (Mono 8, Mono 16, color)
        std::string getVideoType();
        // runs a void I2C command i.e. 0x50007 for FFC
        void runI2CCommand(uint32_t cmd);
        // gets integer result of an I2C command
        uint32_t getI2CInt(uint32_t cmd);
        // gets string result of an I2C command
        std::string getI2CString(uint32_t cmd);
        // sets command for I2C 
        void setI2CInt(uint32_t cmd, uint32_t val);
        // captures still image
        void captureImage(std::string filename);
    private:
        int i2cDevice = -1;
        int sensorAddress = -1;

        NvMainContext mainCtx;
        bool getI2CInfo(char *filename, int *deviceHandle, int *sensorHandle);
        std::string ColorToString(FLIR_COLOR val);
        std::string FFCModeToString(FLIR_FFCMODE val);
        std::string VideoTypeToString(FLIR_VIDEO val);
};

}

#endif