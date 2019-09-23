cmake_minimum_required (VERSION 3.1)
project(opencvConnector)

set(CompilerPrefix /usr/bin/aarch64-linux-gnu-)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/../cv_install/lib/cmake/opencv4)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_COMPILER ${CompilerPrefix}g++)
set(CMAKE_CXX_FLAGS "-std=c++11")

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ../utils)

add_library(opencvConnector SHARED
    opencvConnector.cpp
    opencvWrapper.cpp
    opencvRecorder.cpp
)

target_link_libraries(opencvConnector ${OpenCV_LIBS})

