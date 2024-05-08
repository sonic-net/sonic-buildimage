#!/bin/bash
modprobe -r i2c_ismt
sleep 0.1
modprobe -r i2c-i801
sleep 0.1
