#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
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


"""
    Mellanox SED (Self-Encrypting Drive) password management.
"""


import subprocess
from sonic_platform_base.sed_mgmt_base import SedMgmtBase


READ_DEFAULT_SED_PW_SCRIPT = '/usr/local/bin/read_default_sed_pw_from_tpm.sh'


class SedMgmt(SedMgmtBase):
    """Mellanox SED password management."""

    _instance = None

    @staticmethod
    def get_instance():
        """Return the SedMgmt singleton instance."""
        if SedMgmt._instance is None:
            SedMgmt._instance = SedMgmt()
        return SedMgmt._instance

    def get_min_sed_password_len(self):
        return 8

    def get_max_sed_password_len(self):
        return 124

    def get_default_sed_password(self):
        """Read default SED password from TPM bank 3 via platform script."""
        try:
            result = subprocess.run(
                [READ_DEFAULT_SED_PW_SCRIPT],
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode == 0 and result.stdout:
                return result.stdout.strip()
        except Exception:
            pass
        return None
