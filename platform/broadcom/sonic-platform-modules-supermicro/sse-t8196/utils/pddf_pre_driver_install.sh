#!/bin/bash

modprobe i2c-i801
modprobe i2c-stub chip_addr=0x50 
modprobe i2c-ismt delay=100
modprobe i2c-dev
