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
"""Python side of the component harness: one line per step into argv[2]."""
import configparser, json, os, subprocess, sys
# The trees the run imports from: a sonic-platform-common checkout, and the
# platform/mellanox/mlnx-platform-api of the sonic-buildimage this crate follows.
try:
    sys.path[:0] = [os.environ['PLATFORM_COMMON_DIR'], os.environ['MLNX_PLATFORM_API_DIR']]
except KeyError as e:
    sys.exit('driver.py: {} is not set; see README.md'.format(e))
import sonic_platform.component as C
from sonic_platform.device_data import DeviceDataManager

KINDS = {
    'ONIE': lambda i: C.ComponentONIE(), 'SSD': lambda i: C.ComponentSSD(), 'BIOS': lambda i: C.ComponentBIOS(),
    'BIOSSN2201': lambda i: C.ComponentBIOSSN2201(), 'CPLD': C.ComponentCPLD, 'CPLDSN2201': C.ComponentCPLDSN2201,
    'CPLDSN4280': C.ComponentCPLDSN4280, 'FPGADPU': C.ComponenetFPGADPU,
}
out = open(sys.argv[2], 'w')
objs = {}


def raise_(e):
    return 'raise {}: {}'.format(type(e).__name__, e)


for i, st in enumerate(json.load(open(sys.argv[1]))):
    print('=== STEP {}'.format(i))
    op = st[0]
    try:
        if op == 'new':
            objs[st[1]] = KINDS[st[2]](st[3])
            r = 'ok'
        elif op == 'host_list':
            lst = []
            try:
                lst.append(C.ComponentONIE())
                lst.append(C.ComponentSSD())
                lst.append(DeviceDataManager.get_bios_component())
                lst.extend(DeviceDataManager.get_cpld_component_list())
                r = [(type(o).__name__, o.get_name()) for o in lst]
            except Exception as e:
                r = '{} | partial={!r}'.format(raise_(e), [(type(o).__name__, o.get_name()) for o in lst])
        elif op == 'call':
            r = getattr(objs[st[1]], st[2])(*st[3])
        elif op == 'shell':
            subprocess.call(['/bin/sh', '-c', st[1]])
            r = 'ok'
        elif op == 'ini':
            try:
                cp = configparser.ConfigParser()
                cp.read(st[1])
                r = []
                for sec, opt in st[2]:
                    try:
                        got = cp.get(sec, opt)
                    except Exception as e:
                        got = raise_(e)
                    r.append((cp.has_option(sec, opt), got))
            except Exception as e:
                r = raise_(e)
        else:
            r = 'bad op'
        line = repr(r)
    except Exception as e:
        line = raise_(e)
    out.write('STEP {} => {}\n'.format(i, line))
    out.flush()
