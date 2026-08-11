# Sensor Bank read Functionality

The board has an 8 bit selector that is enabled one bit at a time by setting
three GPIO outputs. When a bit is enabled (enable goes low) the input is read
on a GPIO input. The simplest use of the sensor bank is to detect a switch
closed condition on each sensor. However, the sensor board connects to the
selector and uses the enable to enable a receiver that can be used to read the
input from a sonar range detector or other 1 bit input device.

For the sonar receiver the input is 9600 BAUD inverted serial. For the LiDAR
the input is 115200 BAUD serial. For these inputs, a PIO serial receiver is
used.

The GPIO used for the read is actually an ADC input, so it is possible
that analog sensors could be read.

The sensor board uses 7 of the 8 selector enables. It leaves S0 unused, so
selecting S0 effectively disables the sensor inputs. The sensor board provides
5V power on the connectors for S3-S7 and a 5V tolerant input. S1 and S2 are
unbuffered connectors with a diode on the drive/enable side.

The Housekeeping run is used to sequentially set the three selector address
outputs. When disabled, S0 is selected. When enabled, S1-S4 are selected
sequentially for 'binary' inputs. S5-S7 are Serial Inputs. For these, more
than one Housekeeping period are used to enable a sensor input and receive
the serial data using a PIO.

The GPIO's are sequential:

* GPIO20 = A0
* GPIO21 = A1
* GPIO22 = A2
* GPIO26_ADC0 = Input

HISTORY:
A PIO program is used to enable a sensor and read one bit of input.
The input is shifted into the ISR. The PIO collects six binary sensor
states. When six bits have been read in the ISR is pushed, so that a DMA can
pull it and put it into a memory location. The sensor board does not use
enable-0, so that can be used as an idle stop point (it will always read as a
one/high). Enables 6 and 7 are used for the sonar serial in. These will be
enabled one at a time and a PIO will be used to read the serial data to
collect two samples from each.
is generated.
