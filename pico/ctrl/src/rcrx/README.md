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

ZZZ - EXPLAIN

### Data Reception and Collection

When started for a new message, the PIO RX program will monitor the line,
waiting for the idle period between messages. Once the idle period is detected
it starts looking for the first start bit (signaling a new message). It then
collects data into the ISR and pushes good data into the RX-FIFO. Data available
triggers a DMA transfer out of the RX-FIFO. The target of the transfer varies
depending on the protocol. The details are explained in the next section.

The PIO RX program will raise an interrupt if a framing or parity error occurs
during receipt of the message. The system tracks the volume of parity/framing
errors and goes back to BAUD and Protocol detection if too many are received.

### Message Reception and End-Of-Message

SBUS messages are 25 bytes long (if valid). This allows configuring the DMA
for 25 bytes to transfer from the RX-FIFO directly into the receive buffer.

SRXL2 messages vary in length (though they will not exceed 80 bytes). To
handle this, the received bytes are sent to a second PIO-SM that monitors the
message. The second SM pro
Therefore, the DMA will be configured for 80 bytes, but in most cases the
message will be shorter. To handle this, the processor will need to end the
DMA when it receives the interrupt from the PIO.

With either protocol, if an error occurs (framing or parity) the processor
will need to end the DMA. It will also increment a counter that can be checked
by other processes.

### Device and BAUD Detection

The module will attempt to detect the serial data BAUD rate, and once
determined, the device type will be deduced.

* 400,000 is SRXL2
* 115,200 is SRXL2
* 100,000 is SBUS

If the BAUD is 115,200 or 100,000, a further check can be done to verify the
protocol.

* 1 Start, 8 Data, No Parity, 1 Stop = SRXL2
* 1 Start, 8 Data, Even Parity, 2 Stop = SBUS

In addition, SBUS messages have a header byte of 0x0F (0b00001111) that can
be used as part of the verification. Including the start, parity, and stop
bits, this would look like:

IDLE-S-  DATA  -P-SS
1111-0-00001111-0-11

The same bit pattern will not be seen with SRXL2, as the Parity Bit in SBUS
would be the Stop Bit in SRXL2, and therefore would be a '1'.
