#!/bin/bash

modprobe i2c-i801_t8164
modprobe i2c-ismt bus_speed=400
modprobe ast