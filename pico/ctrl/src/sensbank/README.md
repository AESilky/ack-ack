# Sensor Bank read Functionality

The board had an 8 bit selector that is enabled one bit at a time by setting
three GPIO outputs. When a bit is enabled (enable goes low) the input is read
on a GPIO input. The simplest use of the sensor bank is to detect a switch
closed condition on each sensor. However, the sensor board connects to the
selector and uses the enable to enable a receiver that can be used to read the
input from a sonar range detector.

For the sonar receiver the input is 9600 BAUD inverted serial.

The GPIO used for the read is actually an ADC input, so it is possible
that analog sensors could be read.

A PIO will be used to sequentially set the three selector outputs and read
the input. The input will be shifted into the ISR. Since the rover will use
two sonar range detectors, the PIO will collect six binary six binary sensor
states. When six bits have been read in the ISR is pushed, so that a DMA can
pull it and put it into a memory location. The sensor board does not use
enable-0, so that can be used as an idle stop point (it will always read as a
one/high). Enables 6 and 7 are used for the sonar serial in. These will be
enabled one at a time and a PIO will be used to read the serial data to
collect two samples from each.
is generated.

The GPIO's are sequential:

* GPIO20 = A0
* GPIO21 = A1
* GPIO22 = A2
* GPIO26_ADC0 = Input

