#!/usr/bin/env python3
#
# __init__.py
#
# Platform-specific SONiC platform API for NextHop Aspeed AST2700 BMC
#

try:
    from sonic_platform.platform import Platform
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

