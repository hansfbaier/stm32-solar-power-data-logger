![HAOYU-MINISTM32 energy monitor](https://github.com/hansfbaier/stm32-solar-power-data-logger/blob/master/energy-monitor.gif?raw=true)

# STM32 energy monitor / logger powered by the $32 HAOYU-MINISTM32V

I wrote this device and software to live monitor and log the solar energy production
and household energy consumption, using a compellingly priced board, you can get here:
[HAOYU-MINISTM32V](https://www.hotmcu.com/hyministm32v-dev-board-32-tft-lcd-module-p-5.html)

I attached this device via GPIO in to the pulse outputs of the two Eastron SDM120 energy meters, which
meter the production and consumption circuits. The code is adapted to the pulse rates of those
meters. If you have different meters, you will have to adapt the macro WATT_HOURS_PER_IMP to the rate of your meter.

I connected this to a Raspberry pi via serial interface, which reads out
the production/consumption values of the current day and uploads those to pvoutput.org
(but the raspi-sourcecode is not part of this repository).
Also the setting of the board clock is done per serial interface.
(You will have to read the source code to figure out how it works).

The code I wrote is covered by the license in the LICENSE file,
all pieces of code adapted/copied from other projects are covered by their respective licenses.

I wrote this using the arm-none-eabi-gcc toolchain, but an older version.
On the most recent toolchain version the code crashes with a hardfault.
I have not been willing to set aside the time to find out why,
so I just keep compiling using the old tool chain version.
(Which I currently don't know, because the computer power supply of that PC has died and 
I did not have to recompile for months now, as it runs stable).

This is also probably not the very latest version, since it resides on the PC hard disk which has died,
but I will update as soon as I feel inclined to get that PC running again.
I published this because the code has been working well for years now, and I hope some other people
might benefit from it.

Best regards,
Hans


