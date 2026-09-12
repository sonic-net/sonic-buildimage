#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

try:
    from sonic_platform_base.bmc_watchdog import BMCWatchdog
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Watchdog(BMCWatchdog):
    """
    Watchdog for the NVIDIA AST2700 BMC.

    `/dev/watchdog0` admits a single opener and is owned by the
    hw-watchdog-mgrd daemon (shipped in the aspeed-platform-services package),
    which pets it and serves arm/disarm/status requests over
    `/run/hw-watchdog-mgrd/hw-watchdog-mgrd.sock`.

    All behaviour is inherited from `BMCWatchdog`, which speaks that IPC
    protocol and falls back to a read-only sysfs check for `is_armed()` when
    the daemon is unreachable. This subclass exists only to expose the
    platform-API-conventional `sonic_platform.watchdog.Watchdog` name.
    """
