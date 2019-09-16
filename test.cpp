#include <iostream>
#include <thread>
#include <chrono>

#include "nvidiaInterface.h"

int main(int argc, char **argv) {
    BosonAPI::NvidiaInterface interface;

    BosonAPI::CmdArgs args;
    memset(&args, 0, sizeof(BosonAPI::CmdArgs));
    args.displayId = 1;
    args.logLevel = 0;
    args.regFile = "boson640.script";
    std::thread runThread(interface.run, args);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "Sending FFC" << std::endl;
    interface.ffc();

    runThread.join();
}