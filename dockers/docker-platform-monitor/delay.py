#!/usr/bin/env python3

# This file serves as a monitor for advanced reboot. 
# Once it exits, the delayed daemon will initiate.

from swsscommon.swsscommon import RestartWaiter
RestartWaiter.waitAdvancedBootDone()
