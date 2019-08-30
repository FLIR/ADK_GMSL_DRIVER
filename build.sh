#!/bin/bash

pushd build
cmake ..
make
cp libopencvConnector.so ..
popd
make