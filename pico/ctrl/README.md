# Controller Board

Controls serial Bus Servos (from HiWonder) that drive and steer the rover.
Also reads up to eight binary sensors. These sensors could possibly be
analog.

The Bus Servo power is controlled in two banks. Each bank can be comprised
of multiple servos. The bank current can be sensed as a whole.

There is an input pin that can be used to read the input of a Spektrum or
FrSky serial receiver.

There is an I2C output that goes to multiple connectors for use in reading
sensors or other devices.

There are two LEDs in addition to the Pico LED.

The Pico UART0 is connected to a USB-Serial device to be used to communicate
with the host.

The Pico UART1 is used to communicate with the Bus Servos.

There is a LCD display panel that can display status and information from
the host.

There is a rotary control 'knob' and five 'cursor' switches that allow
interaction with the device.
