#!/bin/bash

modprobe -r ast
modprobe i2c-i801_t8164
modprobe ast