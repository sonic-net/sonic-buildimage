# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# BSP packages.

CISCO_WB_BSP := cisco-wb-bsp_$(CISCO_WB_BSP_VERSION)_amd64.deb
CISCO_WB_BSP_PYLIB := cisco-wb-bsp-pylib_$(CISCO_WB_BSP_VERSION)_amd64.deb

CISCO_WB_BSP_ACPI_PID_DEBS := \
    cisco-wb-acpi-8102-64h-o_$(CISCO_WB_BSP_VERSION)_amd64.deb \
    cisco-wb-acpi-8201-32fh-o_$(CISCO_WB_BSP_VERSION)_amd64.deb \
    cisco-wb-acpi-8101-32fh-o-c01_$(CISCO_WB_BSP_VERSION)_amd64.deb

CISCO_WB_BSP_8102_64H_O := \
    cisco-wb-8102-64h-o_$(CISCO_WB_BSP_VERSION)_amd64.deb
CISCO_WB_BSP_8201_32FH_O := \
    cisco-wb-8201-32fh-o_$(CISCO_WB_BSP_VERSION)_amd64.deb
CISCO_WB_BSP_8101_32FH_O_C01 := \
    cisco-wb-8101-32fh-o-c01_$(CISCO_WB_BSP_VERSION)_amd64.deb
CISCO_WB_BSP_8000 := \
    cisco-wb-8000_$(CISCO_WB_BSP_VERSION)_amd64.deb

CISCO_WB_BSP_PID_DEBS := \
    $(CISCO_WB_BSP_8102_64H_O) \
    $(CISCO_WB_BSP_8201_32FH_O) \
    $(CISCO_WB_BSP_8101_32FH_O_C01) \
    $(CISCO_WB_BSP_8000)

$(foreach deb,$(CISCO_WB_BSP) $(CISCO_WB_BSP_PYLIB) \
                 $(CISCO_WB_BSP_ACPI_PID_DEBS) $(CISCO_WB_BSP_PID_DEBS), \
    $(eval $(call add_prebuilt_debs,$(deb),$(CISCO_WB_URL))))

$(foreach deb,$(CISCO_WB_BSP) $(CISCO_WB_BSP_PYLIB), \
    $(eval $(deb)_PLATFORM := $(CISCO_8000_PLATFORMS)))

$(CISCO_WB_BSP_8000)_PLATFORM := $(CISCO_8000_PLATFORMS)

$(eval $(CISCO_WB_BSP_8102_64H_O)_PLATFORM := \
    $(CISCO_8102_64H_O_VARIANT_PLATFORMS))
$(eval $(CISCO_WB_BSP_8201_32FH_O)_PLATFORM := \
    $(CISCO_8201_32FH_O_VARIANT_PLATFORMS))
$(eval $(CISCO_WB_BSP_8101_32FH_O_C01)_PLATFORM := \
    $(CISCO_8101_32FH_O_C01_VARIANT_PLATFORMS))

# ACPI packages are installed in the base image.
$(foreach deb,$(CISCO_WB_BSP_ACPI_PID_DEBS), \
    $(eval $(deb)_PLATFORM := $(CISCO_8000_PLATFORMS)))
