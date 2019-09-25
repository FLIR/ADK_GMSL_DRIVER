#include <string>
#include <fstream>
#include <regex>
#include <thread>
#include "commandListener.h"
#include "nvidiaInterface.h"

extern "C" {
    #include "opencvConnector.h"
    #include "helpers.h"
}

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
}

NvidiaInterface::~NvidiaInterface() {}

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


void NvidiaInterface::ffc() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    TriggerShutter(i2cDevice, sensorAddress);
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

    uint32_t *sn;
    GetSerialNumber(i2cDevice, sensorAddress, sn);

    return *sn;
}

void NvidiaInterface::setColors(BosonColor color) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    SetColors(i2cDevice, sensorAddress, color);
}

std::string NvidiaInterface::getSceneColor() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    BosonColor color;
    char response[32];
    GetColorMode(i2cDevice, sensorAddress, &color);
    ColorToString(color, response);

    std::string sceneColor(response);

    return sceneColor;
}

void NvidiaInterface::setFfcMode(FFCMode mode) {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return;
    }

    SetFFCMode(i2cDevice, sensorAddress, mode);
}

std::string NvidiaInterface::getFfcMode() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    FFCMode mode;
    char responseStr[32];
    GetFFCMode(i2cDevice, sensorAddress, &mode);
    FFCModeToString(mode, responseStr);
    std::string ffcMode(responseStr);

    return ffcMode;
}

std::string NvidiaInterface::getPartNumber() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    char pn[64];
    GetPartNumber(i2cDevice, sensorAddress, pn);
    std::string pnStr(pn);

    return pnStr;
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

std::string NvidiaInterface::getVideoType() {
    if(i2cDevice == -1 || sensorAddress == -1) {
        LOG_ERR("Application must be running to use command");
        return "";
    }

    VideoType video;
    char responseStr[32];
    GetVideoType(i2cDevice, sensorAddress, &video);
    VideoTypeToString(video, responseStr);
    std::string vidType(responseStr);

    return vidType;
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