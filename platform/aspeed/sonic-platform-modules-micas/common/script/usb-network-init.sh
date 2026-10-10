#!/bin/bash
# Compatibility entry point for the Micas USB gadget service.

exec /usr/local/bin/micas-usb-network-init.sh "$@"
