#!/bin/bash

modprobe i2c-i801
modprobe ast
modprobe i2c-ismt delay=100
modprobe i2c-dev
