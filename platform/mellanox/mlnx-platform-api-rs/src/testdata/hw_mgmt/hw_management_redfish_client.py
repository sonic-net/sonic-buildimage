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
"""A stand-in for hw-management's hw_management_redfish_client: BMCAccessor
with the two methods bmc.py calls.  The class attributes say what they do;
login() prints, as hw-management's reports its flow."""


class BMCAccessor:
    password = 'tpm-derived'
    password_error = None
    login_ret = 0
    login_error = None
    logins = 0

    def get_login_password(self):
        if BMCAccessor.password_error:
            raise Exception(BMCAccessor.password_error)
        return BMCAccessor.password

    def login(self, password=None):
        BMCAccessor.logins += 1
        print('Login to BMC')
        if BMCAccessor.login_error:
            raise Exception(BMCAccessor.login_error)
        print(f"-- BMC Login {'Pass' if BMCAccessor.login_ret == 0 else 'Fail'}, Flow: A")
        return BMCAccessor.login_ret
