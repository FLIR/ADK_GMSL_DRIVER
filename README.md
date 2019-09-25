# NVIDIA - Boson SDK

## Summary
This is the NVIDIA-Boson SDK. This provides a set of functions to visualize Boson video and to communicate with the camera. This is intended to work with an Nvidia Drive AGX/Xavier platform with the Boson camera connected via GMSL. The communication protocol is GMSL-2. All commands to the Boson are sent via I2C. 

## Requirements

### Host machine

- Ubuntu >= 16.04 (may work on lower versions - untested)
- aarch64 cross-compilation tools and dependencies
```
> sudo apt-get update
> sudo apt-get install gcc g++ make gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
> sudo apt-get install build-essential autoconf libtool cmake pkg-config git python-dev swig3.0 libpcre3-dev nodejs-dev
```
### Target machine

- Nvidia Drive AGX/Xavier
    - Ubuntu >= 16.04
    - aarch64 CPU architecture

## Target machine setup

Ensure that you can ssh into the target machine. If you have not yet done so, follow these [instructions](https://developer.nvidia.com/drive/learn/tutorial-ssh) to set up ssh communication

## Building the executable

Open a terminal and navigate to this directory. Then run the build script
```
> cd <current directory>
> mkdir build
> chmod +x build.sh
> ./build.sh
```
This produces an executable called nvmimg_cc

## Running the executable

First, transfer the folder from the host to the target machine
```
> scp ../<name of folder> nvidia@<IP address of target>:/tmp
```
Next, on the target machine, move the folder to a useful location
```
> mv /tmp/<name of folder> <project path>
```
Then, run the application
```
> cd <name of folder>
> sudo LD_LIBRARY_PATH=$PWD ./nvmimg_cc -wrregs boson640.script -d 0
```
This will run the camera in 8-bit video mode and display with an OpenCV window. The included boson640.script and boson640_16.script set up the camera for 8-bit and 16-bit video modes respectively. See [here](https://docs.nvidia.com/drive/active/5.1.0.2L/nvvib_docs/index.html#page/DRIVE_OS_Linux_SDK_Development_Guide%2FNvMedia%2Fnvmedia_nvmimg_cc.html%23wwpID0E0PB0HA) for more information on the script file syntax.

## Further Development
This code provides a C++ interface for interacting with the Nvidia backend. The NvidiaInterface (nvidiaInterface.h) object provides a set of functions for interacting with the camera.

## Getting help
Please contact Anil Dhurjaty (anil.dhurjaty@flir.com), Kelsey Judd (kelsey.judd@flir.com), or Andres Prieto-Moreno (andres.Prieto-Moreno@flir.com) for any technical questions.






