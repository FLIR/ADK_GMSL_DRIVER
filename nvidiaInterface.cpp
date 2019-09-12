#include "nvidiaInterface.h"

extern "C" {
    #include "main.h"
}

NvidiaInterface::NvidiaInterface() {}
NvidiaInterface::~NvidiaInterface() {}
void NvidiaInterface::run(int argc, char **argv) {
    Run(argc, argv);
}

int main(int argc, char **argv) {
    NvidiaInterface foo;

    std::cout << "C++!" << std::endl;
    
    foo.run(argc, argv);
}