#!/bin/bash
modprobe -r i2c-ismt
sleep 0.1
modprobe -r i2c-i801

# ker_name=$(uname -r)
# driver_path=/usr/lib/modules/${ker_name}/extra/

sync
