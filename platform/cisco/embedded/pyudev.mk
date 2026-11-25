# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

PYUDEV_VER := 0.24.0-1
PYTHON3-PYUDEV := python3-pyudev_$(PYUDEV_VER)_all.deb
$(PYTHON3-PYUDEV)_URL := http://ftp.us.debian.org/debian/pool/main/p/pyudev/$(PYTHON3-PYUDEV)
SONIC_ONLINE_DEBS += $(PYTHON3-PYUDEV)

SIX_VER := 1.16.0-4
PYTHON3-SIX := python3-six_$(SIX_VER)_all.deb
$(PYTHON3-SIX)_URL := http://ftp.us.debian.org/debian/pool/main/s/six/$(PYTHON3-SIX)
SONIC_ONLINE_DEBS += $(PYTHON3-SIX)
