#!/bin/bash

set -x
set -o
set -e

mkdir -p build
cd build

#cmake ..
make -j 4

openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg \
    -c "program namebadge.elf verify reset exit"
