#!/bin/bash
openocd -f openocd.cfg -c init -c "reset init" -c "halt" &
sleep 3
nc localhost 4444 <<EOF
flash write_image erase main.bin 0x8000000 bin verify
reset run
EOF
pkill openocd
echo Done
