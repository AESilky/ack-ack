'''
Utility program to generate sin, cos, tan table for hiwonder bus-servo
angles, which are 0-1000 = 0-240 deg, or 0.24 deg per unit, or 375=90°.
This will allow Pico to pull a value from a table to compute IK values,
rather than converting to/from radians and performing trig functions.

Copyright 2024 AESilky (SilkyDesign)
'''
import math

def hwians_to_degs(hw):
    degs = (180 / 750) * hw
    return degs

def hwians_to_rads(hw):
    rads = (math.pi / 750) * hw
    return rads

print(";\n;sin")
for hw in range(0, 376, 1):
    rads = hwians_to_rads(hw)
    degs = hwians_to_degs(hw)
    r = math.sin(rads)
    print("{}\t; HW:{:3} Rads:{} Degs:{}".format(r, hw, rads, degs))

print(";\n;cos")
for hw in range(0, 376, 1):
    rads = hwians_to_rads(hw)
    degs = hwians_to_degs(hw)
    r = math.cos(rads)
    print("{}\t; HW:{:3} Rads:{} Degs:{}".format(r, hw, rads, degs))

print(";\n;tan")
for hw in range(0, 376, 1):
    rads = hwians_to_rads(hw)
    degs = hwians_to_degs(hw)
    r = math.tan(rads)
    print("{}\t; HW:{:3} Rads:{} Degs:{}".format(r, hw, rads, degs))
