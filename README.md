# NVIDIA - Boson SDK

## Summary
This is the NVIDIA-Boson SDK. This provides a set of functions to visualize Boson video and to communicate with the camera. This is intended to work with an Nvidia Drive AGX/Xavier platform with the Boson camera connected via GMSL. The communication protocol is GMSL-2. All commands to the Boson are sent via I2C. 

## Requirements

### Host machine

- Ubuntu 16.04
- aarch64 cross-compilation tools and dependencies

Download and install the [Nvidia SDK manager](https://developer.nvidia.com/nvidia-sdk-manager). Install Drive Software 9.0 on the host machine

Install aditional dependencies
```
> sudo apt-get update
> sudo apt-get install libboost-all-dev
```
Install opencv dependencies: follow steps 1 - 3 from [these instructions](https://www.learnopencv.com/install-opencv3-on-ubuntu/)

### Target machine

- Nvidia Drive AGX/Xavier
    - Ubuntu 16.04
    - aarch64 CPU architecture

## Target machine setup

Ensure that you can ssh into the target machine. If you have not yet done so, follow these [instructions](https://developer.nvidia.com/drive/learn/tutorial-ssh) to set up ssh communication

Download the distribution folder and extract it.

## Setting up opencv
You will need to build opencv for cross-compilation. To do this:
```
> cd <dist folder>/opencv
> cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../../toolchains/aarch64-gnu.toolchain.cmake \
    -DBUILD_OPENCV_PYTHON3=ON \
    -DPYTHON_DFAULT_EXECUTABLE=$(which python3) \
    -DCMAKE_INSTALL_PREFIX=../../cv_install ..
> make -j$(nproc)
> make install
```

## Building the executable

Open a terminal and navigate to this directory. Then run the build script
```
> cd <current directory>
> mkdir build
> chmod +x build.sh
> ./build.sh
```
This produces an executable called nvidiaBoson

## Running the executable

If you would like to use `rsync`, you can use the `syncDirs.sh` script. Simply edit the file to change the IP, target username and destination location. Next, run
```
./syncDirs.sh
```

Otherwise you can transfer the contents in the following way:

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
> sudo LD_LIBRARY_PATH=$PWD ./nvidiaBoson -wrregs boson640.script -d 0
```
This will run the camera in 8-bit video mode and display with an OpenCV window. The included boson640.script and boson640_16.script set up the camera for 8-bit and 16-bit video modes respectively. See [here](https://docs.nvidia.com/drive/active/5.1.0.2L/nvvib_docs/index.html#page/DRIVE_OS_Linux_SDK_Development_Guide%2FNvMedia%2Fnvmedia_nvmimg_cc.html%23wwpID0E0PB0HA) for more information on the script file syntax.

## Further Development
This code provides a C++ interface for interacting with the Nvidia backend. The NvidiaInterface (nvidiaInterface.h) object provides a set of functions for interacting with the camera.

Use the `run` method in NvidiaInterface to display streaming video in an OpenCV window. Run `CommandListener::listen` in a separate thread to allow for sending commands via terminal.

### Important files to look at
- nvidiaInterface.cpp
    - File that includes the main entry point for the executable
    - Contains interface for communication with the Boson (frame, telemetry, I2C functions)
    - If you would like to expand the supported I2C functions, the Boson SDK functions are in Client_API.c in the BosonSDK/ClientFiles_C folder. Simply wrap the desired function in an NvidiaInterface method. 
- main.c
    - Entry point for Nvidia C functions
    - `Run` function starts an infinite loop getting and sending frames to OpenCV
- opencvWrapper.cpp
    - Provides functions for interacting with OpenCV
- opencvConnector.cpp
    - Provides a C interface for interacting with the OpenCV wrapper

## Troubleshooting
In testing, I noticed that the Nvidia SDK Manager does not always create a host Drive SDK folder with libraries. If you are getting the error:
```
/usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/bin/ld: cannot find -lnvmedia
```
or something like it on build, then check that the folder `$(HOME)/nvidia/nvidia_sdk/DRIVE_Software_9.0_Linux_hyperion_E3550/DriveSDK` exists. If it does not, then download the [Drive SDK](https://novacoast-my.sharepoint.com/:f:/p/adhurjaty/Eplir5iOVfJDr08M_oNcLJgBzYh35G3fwoH_WS7eRKMWjw?e=HMUx4C) and place it in the path `~/nvidia/nvidia_sdk/DRIVE_Software_9.0_Linux_hyperion_E3550` or place it somewhere and change line 9 of ../make/nvdefs.mk
```
NV_TOPDIR  = "$(HOME)/nvidia/nvidia_sdk/DRIVE_Software_9.0_Linux_hyperion_E3550/DriveSDK"
```
to
```
NV_TOPDIR  = "<SDK location>"
```

## Getting help
Please contact Anil Dhurjaty (anil.dhurjaty@flir.com), Kelsey Judd (kelsey.judd@flir.com), or Andres Prieto-Moreno (andres.Prieto-Moreno@flir.com) for any technical questions.
