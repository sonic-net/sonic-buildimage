#!/bin/bash
# Compatibility entry point for the Micas USB gadget udev setup.

exec /usr/local/bin/micas-usb-network-udev-init.sh "$@"
