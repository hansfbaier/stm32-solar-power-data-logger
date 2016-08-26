#!/bin/bash
openocd -f openocd.cfg -c init -c "reset init" -c "halt" &
sleep 1
arm-none-eabi-gdb --eval-command="target remote localhost:3333" main.elf
