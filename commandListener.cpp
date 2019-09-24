#include <boost/algorithm/string/predicate.hpp>
#include "commandListener.h"

using namespace BosonAPI;

CommandListener::CommandListener(NvidiaInterface *interface) : 
    interface(interface)
{}

CommandListener::~CommandListener() {}

void CommandListener::listen() {
    uint32_t response = 0;
    std::string responseStr;
    char inputParam[32];
    uint32_t inputNums[4];

    while(interface->isRunning()) {
        std::string userInput = interface->getUserInput();
        if(userInput.length > 0) {
            if(boost::iequals(userInput, "f")) {
                interface->ffc();
            } else if(boost::iequals(userInput, "sn")) {
                response = interface->getSerialNumber();
                printf("Serial number: %d\n", response);
            } else if(boost::iequals(userInput, "w")) {
                interface->setColors(WHITE_HOT);
            } else if(boost::iequals(userInput, "b")) {
                interface->setColors(BLACK_HOT);
            } else if(boost::iequals(userInput, "fa")) {
                interface->setFfcMode(AUTO_FFC);
            } else if(boost::iequals(userInput, "fm")) {
                interface->setFfcMode(MANUAL_FFC);
            } else if(boost::iequals(userInput, "c")) {
                responseStr = interface->getSceneColor();
                printf("Color mode: %s\n", responseStr);
            } else if(boost::iequals(userInput, "pn")) {
                responseStr = interface->getPartNumber();
                printf("Part number: %s\n", responseStr);
            } else if(boost::iequals(userInput, "mode")) {
                responseStr = interface->getFfcMode();
                printf("FFC mode: %s\n", responseStr);
            } else if(boost::iequals(userInput, "video")) {
                responseStr = interface->getVideoType();
                printf("Video type: %s\n", responseStr);
            } else if(sscanf(userInput.c_str(), "geti %x", &inputNums[0])) {
                response = interface->getI2CInt(inputNums[0]);
                printf("%d\n", response);
            } else if(sscanf(userInput.c_str(), "gets %x", &inputNums[0])) {
                std::string i2cStr = interface->getI2CString(inputNums[0]);
                printf("%s\n", i2cStr);
            } else if(sscanf(userInput.c_str(), "seti %x %x", &inputNums[0],
                &inputNums[1])) 
            {
                interface->setI2CInt(inputNums[0], inputNums[1]);
            } else if(sscanf(userInput.c_str(), "r %s", inputParam)) {
                interface->startRecording(inputParam);
            } else if(!strcasecmp(userInput.c_str(), "r")) {
                interface->stopRecording();
            } else {
                printf("%s: Unsupported input: %s\n", __func__, userInput);
            }

            interface->flushInput();
        }
    }
}

