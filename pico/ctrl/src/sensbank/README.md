# Sensor Bank read Functionality

The board had an 8 bit selector that is enabled one bit at a time by setting
three GPIO outputs. When a bit is enabled (taken high) the input is read
on a GPIO input. The idea of the sensor bank is to detect a switch closed
condition on each sensor.

The GPIO used for the read is actually an ADC input, so it is possible
that analog sensors could be read.

A PIO will be used to sequentially set the three selector outputs and read
the input. The input will be shifted into the ISR. When all eight bits
have been read in, the byte is made available to the CPU and an interrupt
is generated.

The GPIO's are sequential:

* GPIO20 = A0
* GPIO21 = A1
* GPIO22 = A2
* GPIO26_ADC0 = Input

