#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from .vpd_parser import VpdParser


class DpuVpdParser(VpdParser):
    """DPU Specific VPD parser"""
    def __init__(self, file_path, dpu_name):
        super(DpuVpdParser, self).__init__(file_path=file_path)
        self.dpu_name = dpu_name

    def get_dpu_data(self, key=None):
        """Retrieves VPD Entry for DPU Specific Key"""
        return self.get_entry_value(f"{self.dpu_name}_{key}")

    def get_dpu_base_mac(self):
        """Retrieves VPD Entry for DPU Specific Mac Address"""
        return self.get_dpu_data("BASE_MAC")

    def get_dpu_serial(self):
        """Retrieves VPD Entry for DPU Specific Serial Number"""
        return self.get_dpu_data("SN")

    def get_dpu_revision(self):
        """Retrieves VPD Entry for DPU Specific Revision"""
        return self.get_dpu_data("REV")

    def get_dpu_model(self):
        """Retrieves VPD Entry for DPU Specific Model Number"""
        return self.get_dpu_data("PN")
