# amigaps2

This is a very simple piece of code for Atmel AVR microcontrollers that allows you to use any PS/2 mouse on the Amiga.

I've tested this with an Arduino Nano v3 but everything with an ATmega that has 2 16bit and one 8bit timer should work just as well.

## Building

This project requires avr-gcc and gnumake or compatible tools.

Just run make in the source directoy. make flash makes it easier to program the code into the ATmega but requires avrdude and a compatible ISP programmer.

## Using

Here's the pinout to connect both the PS/2 mouse to the ATmega and the ATmega to the Amiga mouse port.

I'm powering mine through the Amiga's mouse port's 5V supply voltage, but technically the mouse is drawing more power than allowed from that, so do that on your own risk.

I won't be responsible for you damaging your valuable Amiga 500 mainboard by doing this.

Anyway, here's the pinout:

| Function   | ATmega  | Arduino Nano v3 |
| ---------- | ------- | --------------- |
| PS/2 VCC   | VCC     | GND |
| PS/2 GND   | GND     | GND |
| PS/2 DATA  | PD3     | D3  |
| PS/2 CLOCK | PD2     | D2  |
| Amiga H    | PC2*    | A2  |
| Amiga HQ   | PC3*    | A3  |
| Amiga V    | PC0*    | A0  |
| Amiga VQ   | PC1*    | A1  |

* These might be swapped pairwise, I'll have to open my prototype case up to see how i ended up doing it :)

## Disclaimer

This is still lacking a bunch of features and was written rather hastily. Use at your own risk.
