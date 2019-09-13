#include "nvidiaInterface.h"

extern "C" {
    #include "cmdline.h"
    #include "main.h"
}

NvidiaInterface::NvidiaInterface() {}
NvidiaInterface::~NvidiaInterface() {}
void NvidiaInterface::run(TestArgs *args) {
    Run(args);
}

int main(int argc, char **argv) {
    NvidiaInterface interface;
    TestArgs allArgs;

    memset(&allArgs, 0, sizeof(TestArgs));
    if (IsFailed(ParseArgs(argc, argv, &allArgs))) {
        return -1;
    }
    
    interface.run(&allArgs);
}