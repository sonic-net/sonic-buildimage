#!/bin/bash

modprobe i2c-i801
modprobe i2c-stub chip_addr=0x50 
