#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2016-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# docker image for mlnx syncd

DOCKER_SYNCD_PLATFORM_CODE = mlnx
include $(PLATFORM_PATH)/../template/docker-syncd-trixie.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(PYTHON_SDK_API) $(MFT) $(MFT_FWTRACE_CFG) $(IPROUTE2_MLNX)

ifeq ($(ENABLE_ASAN), y)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD_DBG)
endif

$(DOCKER_SYNCD_BASE)_FILES += $(RDB-CLI) $(ISSU_VERSION_FILE)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

ifeq ($(SDK_FROM_SRC), y)
$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(MLNX_SDK_DBG_DEBS) $(MLNX_SAI_DBGSYM)
endif

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

# Grant device-cgroup access previously provided implicitly by --privileged.
# The SX SDK opens the sx_core char device (/dev/sxdevs/sxcdev, major dynamically
# allocated by the driver) and phcsync opens the PTP clock device (/dev/ptp*,
# also dynamically allocated). A blanket rule mirrors the Broadcom SOC-init fix
# and covers both. The cgroup rule only grants permission to use a device node;
# the node must still be present in the container. /dev/sxdevs is bind-mounted by
# docker_image_ctl.j2 (per-ASIC sxcdev for multi-ASIC, the whole directory for
# single-ASIC), and /dev/ptp* is bind-mounted separately because it lives
# directly under /dev rather than inside /dev/sxdevs.
$(DOCKER_SYNCD_BASE)_RUN_OPT += --device-cgroup-rule='a *:* rwm'

# phcsync calls clock_settime() on the ASIC PHC via phc_ctl, which the kernel
# gates on CAP_SYS_TIME. Without it the write fails with EPERM and the ASIC
# clock is never synced to CLOCK_REALTIME. --privileged used to grant this
# implicitly; the capability set kept by the syncd hardening change does not.
$(DOCKER_SYNCD_BASE)_RUN_OPT += --cap-add=SYS_TIME

