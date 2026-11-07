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
"""A stand-in for hw-management's hw_management_independent_mode_update, with
its functions' signatures: each call is recorded, and answers True, as
hw-management's does when it wrote.  A function named in `fail` raises
RuntimeError with the message given instead."""

calls = []
fail = {}


def _record(*call):
    calls.append(call)
    if call[0] in fail:
        raise RuntimeError(fail[call[0]])
    return True


def module_data_set_module_counter(module_counter):
    return _record('module_data_set_module_counter', module_counter)


def thermal_data_set_asic(asic_index, temperature, warning_threshold, critical_threshold, fault=0):
    return _record('thermal_data_set_asic', asic_index, temperature, warning_threshold, critical_threshold, fault)


def thermal_data_set_module(asic_index, module_index, temperature, warning_threshold, critical_threshold, fault=0):
    return _record('thermal_data_set_module', asic_index, module_index, temperature, warning_threshold,
                   critical_threshold, fault)


def vendor_data_set_module(asic_index, module_index, vendor_info=None):
    return _record('vendor_data_set_module', asic_index, module_index, vendor_info)


def thermal_data_clean_asic(asic_index):
    return _record('thermal_data_clean_asic', asic_index)


def thermal_data_clean_module(asic_index, module_index):
    return _record('thermal_data_clean_module', asic_index, module_index)
