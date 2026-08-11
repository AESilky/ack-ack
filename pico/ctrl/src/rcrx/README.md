# RCRX - Remote Control Receiver

Module to process remote control data/commands received from a Spektrum SRXL2
or a FrSKY SBUS (inverted Futaba SBUS) radio receiver.

The received data is stored in a channel buffer and status. When the value of
any channel or a status changes, a message is posted.

SRXL2 messages shall not exceed 80 bytes total, including 3-byte header and 2-byte
CRC. SRXL2 uses 115,200 or 400,000 BAUD.

SBUS messages are 25 bytes long. SBUS uses 100,000 BAUD.

## Implementation Concepts

The module uses a PIO State Machine (SM) and a DMA channel to detect the BAUD
and Protocol of the RC receiver. The next section describes the detection
mechanism.

Once the BAUD and Protocol are detected, 1 or 2 State-Machines (SM) of a PIO
and 2 or 3 DMA channels are used to collect the data from the receiver. The
PIO and DMA collect an entire message into memory with minimal interaction
by the system processor before raising an interrupt signaling the system that
a complete message is ready.

The data is received (via PIO RX program(s) and DMA) into a Receive Buffer until
a full message is collected. Once a full message is collected, DMA transfers
the data from the Receive Buffer to the Current Message Buffer after the Current
Message Buffer is copied to the Previous Message Buffer (via DMA). When a new
message is in the Current Message Buffer an interrupt is raised to alert the
system (processor). The system precesses the raw message data into channel
values and status and then posts a message that RC data is available.

This arrangement allows receiving a complete message with very little involvement
by the system processor after it has set things up. Since SRXL2 messages are
variable length, there is minor system involvement during reception (see below).

### BAUD and Protocol Detection

The module will attempt to detect the serial data BAUD rate, and once
determined, the device type will be deduced.

* 400,000 is SRXL2
* 115,200 is SRXL2
* 100,000 is SBUS

If the BAUD is 115,200 or 100,000, a further check can be done to verify the
protocol.

* 1 Start, 8 Data, No Parity, 1 Stop = SRXL2
* 1 Start, 8 Data, Even Parity, 2 Stop = SBUS

In addition, SBUS messages use inverse signal levels (idle=L, start=H, 0=H, and 1=L).
SBUS also has a header byte of 0x0F (0b00001111) that can be used as part of
the verification. Including the start, parity, and stop bits, this would look like:

IDLE-S-  DATA  -P-SS
0000-1-00001111-1-00

The same bit pattern will not be seen with SRXL2.

### Data Reception and Collection

When started for a new message, the PIO RX program will monitor the line,
waiting for the idle period between messages. Once the idle period is detected
it starts looking for the first start bit (signaling a new message). It then
collects data into the ISR and pushes the data into the RX-FIFO. Data available
triggers a DMA transfer out of the RX-FIFO. The target of the transfer varies
depending on the protocol. The details are explained in the next section.

The PIO RX program will raise an interrupt if a framing or parity error occurs
during receipt of the message. At this point, the PIO-SM stops. The last byte
transferred to the system is the byte the error was detected on.
The system tracks the volume of parity/framing errors and goes back to BAUD
and Protocol detection if too many errors are received within a specific
time period. If an error occurs (framing or parity) the processor needs to
end the DMA. It will also increment a counter that can be checked by other
processes.

### Message Reception and End-Of-Message

#### SBUS

SBUS uses inverted serial signal line levels. The idle line is LOW (typical
serial (RS232, etc.) are HIGH when idle), the start-bit is HIGH, the data
bits are LOW for a '1' and HIGH for a '0', the stop-bits are LOW. This is an
image of a message.

![SBUS Message Frame](images/SBUS-Message-Timing.png)  
*One complete message packet. Note the time between the vertical lines is 3.0ms*

A closer view of the start of a message, showing the start-bit, data, parity,
stop-bits, and continuing on:

![SBUS Data](images/SBUS-StartBit-Timing.png)  
*The Header Byte (0x0F) being received. Followed by the rest of the message. Remember that data is received LSB first.*

The PIO receive program for SBUS handles the inverted nature of the serial
line and also detects parity and framing errors.

##### Parity Error Detection

SBUS uses Even-Parity. This means that the level of the parity bit is set
such that there are an even number of '1' bits in the received data (9 bits).

The PIO program checks the parity as follows:

1. At the beginning of each byte (when the Start-Bit is detected) the OSR is loaded with 0xAAA0|0000 (every other bit is '1' for 12 bits - we need 9 bits for the check).
2. As each data bit is received, it is pushed to the ISR and also checked to see if it is '1'. If it is, one bit is shifted out of the OSR (from the left).
3. After 8 data bits are received and shifted into the ISR, the ISR is pushed and the parity bit is sampled into the X register.
4. A bit is shifted out of the OSR into the Y register. Since bits have been shifted out of the OSR each time a '1' bit has been received, the bit now shifted out into the Y register is the expected parity bit value.
5. The X register (the parity bit actually received) is compared to the Y register (the parity bit expected).
6. If the two are equal, the code continues on to check for the stop bits.
7. If they are not equal, the code puts the X value in the upper four bits of the ISR and the Y value into the lower four bits (of the data byte) of the ISR, and pushes the value. This allows the system to see the data byte that was received, the received parity bit, and the expected parity bit, that resulted in an error being reported.
8. The program then sets IRQ bits 4 and 0 and waits for the system to acknowledge the interrupt.

Initially setting the OSR to 0xAAA0|0000 is done with the following code (as you cannot set a value larger than 31 (0x1F) directly)

    ; Setup for Even-Parity check, put 1010,1010,1010,0000 in the OSR
    set y, 0b10101          ; 1. Start with 10101 (you can set 5 bits max)
    in y, 5                 ; 2. Shift 5 bits in  (ISR has 1010,1000,0000,0000)
    mov y, !y               ; 3. Y gets 1111,1111,1110,1010
    in y, 6                 ; 4.  shift 6 bits in (ISR has 1010,1010,1010,0000)
    mov osr, isr            ; 6. Put the 1010,1010,1010,0000 into the OSR
    mov isr, NULL [20 - 7]  ; 7. Clear ISR (also clears ISR bit-count) - Delay 1 bit

##### DMA from PIO-SM to Buffer

SBUS messages are 25 bytes long (if valid). This allows configuring the DMA
for 25 bytes to transfer from the RX-FIFO directly into the receive buffer.
Therefore, for SBUS the DMA End of Transfer IRQ is used to signal that a
message has been received and is ready to be processed.

#### SRXL2

SRXL2 messages vary in length (though they will not exceed 80 bytes). To
handle this, the received bytes are sent to a second PIO-SM that monitors the
message. The second SM raises IRQ-2 when the message has been received (IRQ-1
and IRQ-5 are used to signal errors).

Therefore, the PIO to Memory DMA will be configured for 80 bytes, but in most
cases the message will be shorter. To handle this, the processor will need to
end the DMA when it receives the interrupt from the PIO.

There is also a DMA that is configured to run forever that transfers the
bytes received by the first PIO-SM to the second PIO-SM.

With either protocol, if an error occurs (framing or parity) the processor
will need to end the DMA. It will also increment a counter that can be checked
by other processes.

#### Common

##### Unique Message Detection

Since SBUS messages reflect the state of the controls of the RC Transmitter, and
also because they continue to be received when the transmitter is off (as long
as it was connected at some point in time during this power-on period), it is
common for the received data to be the same for a number of consecutive messages
(in fact, all of the messages will be the same after the transmitter is turned
off). To account for this, and keep the system from performing a lot of unnecessary
work, the DMA's capability to sniff the data as it's being transferred and to
calculate a CRC-32 value is used to detect when two consecutive messages are the
same.

##### Received (Enqueue) to Current Buffer Transfer

Received data, either SBUS or SRXL2, is put into the Enqueue buffer. At the point
that a complete message has been received (25 bytes for SBUS, as signalled by
the PIO for SRXL2), and it is determined that it isn't a duplicate of the
previous message, the Current Message buffer is copied to the Previous Message
buffer and the Enqueue buffer is copied to the Current Message buffer to be
processed. This is done in a single DMA operation by copying in reverse through
all three buffers (starting at the end of the buffers and working toward the
beginning for 2X the buffer length).

When the buffer copy DMA completes, it raises an interrupt to initiate the
processing from the received message data to the state of the RC Channels.

## System Remote Control Value Representation

Since SBUS and SRXL2 are both supported, yet the two don't have the same channel
value and other value representations, this module uses a locally defined (system)
representation for the values. The System Value Representation is easy to use and
easy to convert either SBUS or SRXL2 channel values to.

### Channel Value

Channel values use an unsigned 16-bit value. The value 0x8000 is the middle/
neutral value. Other important points are shown below:

| Value     | Key Attribute                                                     |
|-----------|-------------------------------------------------------------------|
| 0xFFFF    | 200% of typical max control travel                                |
| 0xA000    | 150% of typical max control travel                                |
| 0xC000    | 100% of typical max control travel (stick full up/right)          |
| 0x8000    | 0 or Neutral/Middle control position                              |
| 0x4000    | -100% of typical minimum control position (stick full down/left)  |
| 0x2000    | -150% of typical minimum control position                         |
| 0x0000    | -200% of typical minimum control position                         |

### RSSI Value

The RSSI value is a signed 8-bit value:

1. RSSI > 0 : % of receiver max R
2. RSSI = 0 : Not connected to transmitter
3. RSSI < 0 : Strength in dBm

### Temperatures

Telemetry temperature values are signed 16-bit values representing 10x the
temperature in degrees Celsius (centigrade).

### Voltages

Telemetry voltage values are signed 8-bit values representing 10x the voltage.
