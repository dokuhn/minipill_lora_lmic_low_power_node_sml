# MiniPill LoRa Hardware version 1.x

Please look at https://www.iot-lab.org/blog/370/ for general information on this
project. In this this file I will share some software specific information.
All hardware versions 1.x are supported by this software


## Update 2021-12-18
Updated STM32LowPower and STM32RTC due to updated STM32 framework for PlatformIO

## Wakeup with external button instead of timed wakeup
In this code I use the LMIC library and use a external switch with debounce circuit
to wake up the MiniPill Lora. The LowPower library uses attachInterruptWakeup to wakeup
on a designated pin.

The first message on boot is always send, due to OTAA handshake. This code can be
used with both ABP and OTAA activation.

Messages will not be send after every press on the button. There has to be a timeout between them.
In my experiments it took about 8 seconds for the controller to finish one button event. But due to the LoRaWAN regulations it is not advisable to send data with very little pause between them.
When the button is pushed multiple times in a short period, only 1 message will be send.

## Very low Power
The power consumption is about 0.5uA when in deepSleep mode, even lower than the 1.5uA
in timed wakeup.
