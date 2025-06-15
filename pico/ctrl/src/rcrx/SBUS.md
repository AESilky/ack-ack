# SBUS Protocol Details

## SBUS Line Protocol

The SBUS Protocol is a serial protocol that was developed by Futaba for hobby remote control applications. It is derived from the RS232 protocol but the voltage levels are inverted. The protocol provides 16 channels of 11 bits each, two digital channels, and two flags for "frame lost" and "failsafe".

For SBUS, a serial port has to be configured as follows:
100'000 Baud rate (this is a non standard baud rate!)
8E2 configuration, i.e.:
1 start bit
8 Data bits
1 Even parity bit
2 Stop bits
Note: The voltage levels of Futaba SBUS are inverted. So while a 0 with a normal serial port is encoded with a low voltage, it is encoded with a high voltage with SBUS. FrSKY SBUS uses standard logic levels.

Note: Even parity means that, for a given set of bits, the occurrences of bits whose value is 1 is counted. If that count is odd, the parity bit value is set to 1, making the total count of occurrences of 1s in the whole set (including the parity bit) an even number. If the count of 1s in a given set of bits is already even, the parity bit's value is 0.

## SBUS Message

A single SBUS message is 25 bytes long and therefore, with the configuration described above, takes 3ms to be transmitted. It consists of the following bytes:

1 Header byte 00001111b (0x0F)
16 * 11 bit channels -> 22 bytes
1 Byte with two digital channels (channel 17 and 18) and "frame lost" and "failsafe" flags
1 Footer byte 00000000b (0x00)
Each byte is composed of 8 bits with IDs as follows [7 6 5 4 3 2 1 0] where bit 0 is the least significant bit. The data of the 16 channels are distributed onto the 22 data bytes starting with the least significant bit of channel 1 as follows (using the notation CHANNEL.BIT_ID):

data byte 00 (01): [01.07 01.06 01.05 01.04 01.03 01.02 01.01 01.00]
data byte 01 (02): [02.04 02.03 02.02 02.01 02.00 01.10 01.09 01.08]
data byte 02 (03): [03.01 03.00 02.10 02.09 02.08 02.07 02.06 02.05]
data byte 03 (04): [03.09 03.08 03.07 03.06 03.05 03.04 03.03 03.02]
data byte 04 (05): [04.06 04.05 04.04 04.03 04.02 04.01 04.00 03.10]
data byte 05 (06): [05.03 05.02 05.01 05.00 04.10 04.09 04.08 04.07]
data byte 06 (07): [06.00 05.10 05.09 05.08 05.07 05.06 05.05 05.04]
data byte 07 (08): [06.08 06.07 06.06 06.05 06.04 06.03 06.02 06.01]
data byte 08 (09): [07.05 07.04 07.03 07.02 07.01 07.00 06.10 06.09]
data byte 09 (10): [08.02 08.01 08.00 07.10 07.09 07.08 07.07 07.06]
data byte 10 (11): [08.10 08.09 08.08 08.07 08.06 08.05 08.04 08.03]
data byte 11 (12): [09.07 09.06 09.05 09.04 09.03 09.02 09.01 09.00]
data byte 12 (13): [10.04 10.03 10.02 10.01 10.00 09.10 09.09 09.08]
data byte 13 (14): [11.01 11.00 10.10 10.09 10.08 10.07 10.06 10.05]
data byte 14 (15): [11.09 11.08 11.07 11.06 11.05 11.04 11.03 11.02]
data byte 15 (16): [12.06 12.05 12.04 12.03 12.02 12.01 12.00 11.10]
data byte 16 (17): [13.03 13.02 13.01 13.00 12.10 12.09 12.08 12.07]
data byte 17 (18): [14.00 13.10 13.09 13.08 13.07 13.06 13.05 13.04]
data byte 18 (19): [14.08 14.07 14.06 14.05 14.04 14.03 14.02 14.01]
data byte 19 (20): [15.05 15.04 15.03 15.02 15.01 15.00 14.10 14.09]
data byte 20 (21): [16.02 16.01 16.00 15.10 15.09 15.08 15.07 15.06]
data byte 21 (22): [16.10 16.09 16.08 16.07 16.06 16.05 16.04 16.03]

The digital channels and flag bytes is composed as:
flag byte: [0 0 0 0 failsafe frame_lost ch18 ch17]
Since the least significant bit is sent first over the serial port, the following bit sequence is transmitted:

shhhhhhhhpss | s 1.0 1.1 1.2 1.3 1.4 1.5 1.6 1.7 pss | s 1.8 1.9 1.10 2.1 2.2 2.3 2.4 pss | ...

### Channel Values

FrSky receivers will output a range of 172 - 1811 with channels set to a range of -100% to +100%. Using extended limits of -150% to +150% outputs a range of 0 to 2047, which is the maximum range achievable with 11 bits of data.

Each of the 16 channels use values in the range of 192 - 1792 which are mapped linearly in the Betaflight Firmware to values in the range 1000 - 2000. These values in the range [1000, 2000] are also what can be observed in the Betaflight Configurator's Receiver tab. Note that e.g. a Taranis transmitter sends values in a slightly larger range than [192, 1792] but these values will later be cropped.
