#!/bin/bash

echo "Compiling Microkernel Simulator..."
g++ -std=c++14 -pthread main.cpp kernel/*.cpp ipc/*.cpp services/*.cpp user/*.cpp -o simulator

if [ $? -eq 0 ]; then
    echo "Compilation Successful!"
    echo "Running Simulator..."
    ./simulator
else
    echo "Compilation Failed!"
fi
