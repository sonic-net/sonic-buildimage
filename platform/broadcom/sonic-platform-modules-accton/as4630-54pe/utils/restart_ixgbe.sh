#!/bin/bash

modprobe -r ixgbe
udevadm control --reload-rules
udevadm trigger
modprobe ixgbe

