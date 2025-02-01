# Ack-Ack Goals and Possible Features

Ack-Ack is intended to be a fun and educational robot. It has the following goals:

Operations:

1. Be able to operate outdoors
1. Be controllable
1. Follow planned missions
1. Function autonomously (at least for short periods)
1. Charging dock that it can locate

## Features/Technology

1. Use multi-leg walking motive (rather than wheels or tracks)
1. ROS
1. ARDUPILOT Mission Planner (?)

## Proposed Sensors

1. Measure current of leg servo motors to detect obstacles
1. Use force sensors for leg contact with ground and touching objects
1. RPi Sense-Hat(?)
1. Ultrasonic range finder
1. Laser range finder
1. 360° LiDAR
1. Camera (RPi)
1. Camera (FPV)
1. GPS
1. Compass
1. Temperature (ambient)
1. Sound (ReSpeaker)

## Proposed User Interface

1. Push-buttons (maybe the switch-board or µKOB?)
1. Rotary encoder (maybe the switch-board or µKOB?)
1. IR Remote
1. Listen to RC receiver for manual control
1. Pluggable mini-display on each leg (to see monitoring info)
1. Buzzer
1. Main CPU (Raspberry Pi) shutdown (and sleep) switch

## Software Controls

1. Main CPU can request peripheral systems to sleep
1. Main CPU can cause peripheral systems to shutdown
1. Main CPU can cause peripheral systems to reset/restart

