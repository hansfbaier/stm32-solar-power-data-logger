#!/bin/bash
pkill openocd
sleep 1
openocd -f openocd.cfg -c init -c "reset init" -c "halt" &
sleep 1
arm-none-eabi-gdb 
