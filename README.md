# MPPT Based on CH32V203C8T6

## Why use a CH32V203C8T6?
  * RISC-V, so why not? A learning opportunity.
  * Low cost sub $ micro.
  * 144MHz clock speed which gives a good PWM resolution at a reasoble PWM speed (0..2047 at 72kHz). This reduces ripple and gives around 25mV resolution with a 50V input.
  * Sufficient peripherals & DMA.
  * Unlike many RISC Vs, this device supports nested interrupts - probably useful for making a "poor man's thread".

## Toolchain
Developed with MounRiver Studio using a WCH WLinkE debugger.
Host platform Ubuntu 2022.4.


