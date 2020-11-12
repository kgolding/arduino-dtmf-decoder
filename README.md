# DTMF Hardware Decoder

Arduino based, designed to be installed in-line of a SMC Gateway V2H MOTOTRBO lead powered by the GW, and to send DTMF digits via UART (9600 8n1).

Settings can be changed over serial, are these are stored in the EEPROM. There is a simple command interface that can be accessed via a serial terminal.

```
Commands:

	H<CR>		Display help
	?<CR>		Display current settings
	T 1000<CR>	Set threshold to 1000
	N 128<CR>	Set 'n' aka block size
	S 8926<CR>	Set sampling rate
	R 999<CR>	Facotry reset to defaults

The space between the command character and the value is optional.

Settngs are stored in persistent memory.

Output:
When a DTMF digit is detected a new line starting with 'DTMF: '
followed by the single digit and <CR> is sent.
	Example: 	DTMF: 5

```

## Wiring it up

| GW Expansion port pin | Function        | Arduino pin                                        |
| --------------------- | --------------- | -------------------------------------------------- |
| 1                     | Audio input     | A0 (in parallel with repeater) via a 1uf capacitor |
| 2                     | Audio ground    | GND (in parallel with repeater)                    |
| 7                     | UART data to GW | D0 (TX)                                            |
| 8 or 13               | Ground          | GND                                                |
| 11                    | 5V              | 5V                                                 |

The default setting should work on a 16 Mhz 328  based Arduino, and by changing the sampling rate to 4400 it should work on an 8 Mhz CPU.

## Tweaking

I found the default sampling rate and 'n' aka block size to be ok, and I only needed to adjust the threshold to improve the decoding performance.