#!/bin/bash

tmux \
    new-session -s "pico-gdb" 'openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg' \; \
    split-window 'gdb-multiarch -ex "target extended-remote localhost:3333" build/namebadge.elf' \; \
    detach-client

exec tmux attach -t "pico-gdb"
