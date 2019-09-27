#include <string>
#include <fstream>
#include <regex>
#include <thread>
#include "commandListener.h"
#include "nvidiaInterface.h"

extern "C" {
    #include <stdbool.h>
    #include "Client_API.h"
    #include "UART_Connector.h"
    #include "opencvConnector.h"
    #include "helpers.h"
}

// temporarily hard-code I2C port
#define I2C_PORT 0
#define BAUD_RATE 921600

using namespace BosonAPI;

static void
CommandFromInt(uint16_t *dst, uint32_t src) {
    uint8_t tempDst[4];
    LsbToMsbArr(tempDst, src);
    for (size_t i = 0; i < 4; i++) {
        dst[i] = tempDst[i];
    }
}

NvidiaInterface::NvidiaInterface() {
    mainCtx.quit = 0;
    Initialize(I2C_PORT, BAUD_RATE);
}

NvidiaInterface::~NvidiaInterface() {
    Close();
}

void NvidiaInterface::run(CmdArgs args) {
    TestArgs cargs;

    memset(&cargs, 0, sizeof(TestArgs));

    cargs.numSensors = 1;
    cargs.numLinks = 0;
    cargs.numVirtualChannels = 1;
    cargs.numSensors = 1;
    cargs.config[0].isUsed = 1;
    cargs.bufferPoolSize = 5;
    cargs.displayIdUsed = 1;
    cargs.displayEnabled = 1;
    cargs.wrregs.isUsed = 1;

    cargs.logLevel = args.logLevel;
    strcpy(cargs.wrregs.stringValue, args.regFile.c_str());
    cargs.displayId = args.displayId;

    run(&cargs);
}

void NvidiaInterface::run(TestArgs *args) {
    CommandListener listener(this);

    if(args->wrregs.isUsed) {
        getI2CInfo(args->wrregs.stringValue, &i2cDevice, &sensorAddress);
    }

    std::thread mainThread(Run, args, &mainCtx);
    std::thread listenerThread(&CommandListener::listen, &listener);
    mainThread.join();
    listener.stop();
    listenerThread.join();
}

bool NvidiaInterface::isRunning() {
    return !mainCtx.quit;
}

std::string NvidiaInterface::getUserInput() {
    if(!mainCtx.cmd) {
        return "";
    }
    std::string input(mainCtx.cmd);
    return input;
}

void NvidiaInterface::flushInput() {
    memset(mainCtx.cmd, 0, sizeof(mainCtx.cmd));
}

bool NvidiaInterface::getI2CInfo(char *filename, int *deviceHandle, 
    int *sensorHandle)
{
    std::ifstream f;
    f.open(filename);
    if(!f.is_open()) {
        return false;
    }

    while(!f.eof()) {
        std::string line;
        std::getline(f, line);

        std::cmatch matches;
        std::regex re(";\\s*I2C Device:\\s*(\\d+).*$", 
            std::regex_constants::icase);
        if(std::regex_match(line.c_str(), matches, re)) {
            *deviceHandle = atoi(matches[1].str().c_str());
        }

        re.assign(";\\s*Sensor Address:\\s*0[xX]([0-9A-Fa-f]+).*$", 
            std::regex_constants::icase);
        if(std::regex_match(line.c_str(), matches, re)) {
            char *p;
            long res = strtol(matches[1].str().c_str(), &p, 16);
            if(*p == 0) {
                *sensorHandle = (int)(res / 2);
            }
        }
        
        if(*deviceHandle > -1 && *sensorHandle > -1) {
            break;
        }
    }

    return true;
}

void NvidiaInterface::getFrame(uint8_t *frame) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    Opencv_getFrame(frame);
}

void NvidiaInterface::getTelemetry(uint8_t *telemetry) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    Opencv_getTelemetry(telemetry);
}

void NvidiaInterface::ffc() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    bosonRunFFC();
}

void NvidiaInterface::toggleHeater() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    ToggleHeater(i2cDevice, sensorAddress);
}

uint32_t NvidiaInterface::getSerialNumber() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return 0;
    }

    uint32_t sn = 0;
    FLR_RESULT result = bosonGetCameraSN(&sn);
    if(result != R_SUCCESS) {
        LOG_ERR("Error getting value");
        return 0;
    }

    return sn;
}

void NvidiaInterface::setColors(const FLIR_COLOR color) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    FLR_RESULT result = colorLutSetId((FLR_COLORLUT_ID_E)color);
    if(result != R_SUCCESS) {
        LOG_ERR("Error setting value");
    }
}

std::string NvidiaInterface::getSceneColor() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    FLR_COLORLUT_ID_E color;
    FLR_RESULT result = colorLutGetId(&color);
    if(result != R_SUCCESS) {
        LOG_ERR("Error getting value");
        return 0;
    }
    
    return ColorToString((FLIR_COLOR)color);
}

void NvidiaInterface::setFfcMode(FLIR_FFCMODE ffcMode) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    FLR_RESULT result = bosonSetFFCMode((FLR_BOSON_FFCMODE_E)ffcMode);
    if(result != R_SUCCESS) {
        LOG_ERR("Error setting value");
    }
}

std::string NvidiaInterface::getFfcMode() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    FLR_BOSON_FFCMODE_E mode;
    FLR_RESULT result = bosonGetFFCMode(&mode);
    if(result != R_SUCCESS) {
        LOG_ERR("Error getting value");
    }
    
    return FFCModeToString((FLIR_FFCMODE)mode);
}

std::string NvidiaInterface::getPartNumber() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    FLR_BOSON_PARTNUMBER_T pnRes;
    FLR_RESULT result = bosonGetCameraPN(&pnRes);
    if(result != R_SUCCESS) {
        LOG_ERR("Error getting value");
    }
    
    char pn[64];
    sprintf(pn, "%s", pnRes.value);
    std::string pnStr(pn);

    return pnStr;
}

std::string NvidiaInterface::getVideoType() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    FLR_DVO_TYPE_E video;
    FLR_RESULT result = dvoGetType(&video);
    if(result != R_SUCCESS) {
        LOG_ERR("Error getting value");        
    }
    
    return VideoTypeToString((FLIR_VIDEO)video);
}

void NvidiaInterface::runI2CCommand(uint32_t cmd) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }
    uint16_t cmdBody[4];
    CommandFromInt(cmdBody, cmd);

    RunVoidCommand(i2cDevice, sensorAddress, cmdBody, NULL);
}

uint32_t NvidiaInterface::getI2CInt(uint32_t cmd) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return 0;
    }
    uint16_t cmdBody[4];
    CommandFromInt(cmdBody, cmd);

    uint32_t result;
    RunCommandWithInt32Response(i2cDevice, sensorAddress, cmdBody, &result);
    return result;
}

std::string NvidiaInterface::getI2CString(uint32_t cmd) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }
    uint16_t cmdBody[4];
    CommandFromInt(cmdBody, cmd);
    
    char result[32];
    RunCommandWithStringResponse(i2cDevice, sensorAddress, cmdBody, result, 32);
    std::string resString(result);
    return resString;
}

void NvidiaInterface::setI2CInt(uint32_t cmd, uint32_t val) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }
    uint16_t cmdBody[4];
    CommandFromInt(cmdBody, cmd);

    RunVoidCommand(i2cDevice, sensorAddress, cmdBody, &val);
}

uint32_t NvidiaInterface::getFps() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return 0;
    }
    uint32_t fps;

    GetFPS(i2cDevice, sensorAddress, &fps);
    
    return fps;
}

void NvidiaInterface::startRecording(std::string filename) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }
    if(mainCtx.videoEnabled) {
        LOG_WARN("Video already recording");
        return;
    }

    mainCtx.videoEnabled = 1;

    uint32_t fps = getFps();
    Opencv_startRecording(fps, (char *)filename.c_str());
}

void NvidiaInterface::stopRecording() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }
    if(!mainCtx.videoEnabled) {
        LOG_WARN("Video not running");
    }

    mainCtx.videoEnabled = 0;

    Opencv_stopRecording();
}

std::string NvidiaInterface::FFCModeToString(FLIR_FFCMODE val) {
    if(val == MANUAL_FFC) {
        return "Manual";
    }
    if(val == AUTO_FFC) {
        return "Auto";
    }
}

std::string NvidiaInterface::ColorToString(FLIR_COLOR val) {
    if(val == COLOR_WHITEHOT) {
        return "White Hot";
    }
    if(val == COLOR_BLACKHOT) {
        return "Black Hot";
    }
}

std::string NvidiaInterface::VideoTypeToString(FLIR_VIDEO val) {
    if(val == VIDEO_MONO16) {
        return "Mono 16";
    }
    if(val == VIDEO_MONO8) {
        return "Mono 8";
    }
    if(val == VIDEO_COLOR) {
        return "Color";
    }
    if(val == VIDEO_ANALOG) {
        return "Analog";
    }
}

int main(int argc, char **argv) {
    NvidiaInterface interface;
    TestArgs allArgs;

    memset(&allArgs, 0, sizeof(TestArgs));
    if (IsFailed(ParseArgs(argc, argv, &allArgs))) {
        return -1;
    }
    
    interface.run(&allArgs);

    // for running with no command line arguments
    // CmdArgs args;
    // memset(&args, 0, sizeof(CmdArgs));
    // args.displayId = 1;
    // args.logLevel = 0;
    // args.regFile = "boson640.script";
    // interface.run(args);
}