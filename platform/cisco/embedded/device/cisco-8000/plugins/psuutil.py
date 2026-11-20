# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

import os
import os.path
import glob
import yaml
from cisco.pacific.resolver import resolver

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

        with open('/opt/cisco/etc/thermal_zone.yaml') as f:
            self.thermal = yaml.safe_load(f)
        self.power = self.thermal.get('power_devices', [])

        #self.psu_oper_status = "in1_input"
        #self.psu_paths  = [ os.path.dirname(p)
        #                   for p in glob.glob('/sys/class/hwmon/hwmon?/{}'.format(self.psu_oper_status)) ]
        #self.psu_num = len(self.psu_paths)
        #if self.psu_num != 2:
        #    print('?WARNING: i2c topology has not been instantiated')


        self.include_trays = False

        self.keys = [
            'name',
            'presence',
            'power_ok',
            'power_in_ok',
            'fault',
        ]

        self.psu = []
        for tray in self.power:
            if self.include_trays or 'modules' not in tray:
                data = { k: tray.get(k, None) for k in self.keys }
                self.psu.append(data)
            for module in tray.get('modules', []):
                data = { k: module.get(k, None) for k in self.keys }
                self.psu.append(data)
        self.psu_num = len(self.psu)

        self.resolve_keys = [
            'presence',
            'power_ok',
            'power_in_ok',
            'fault',
        ]
        self.psu_obj = []
        for psu in self.psu:
            data = {}
            for k in self.resolve_keys:
                if k in psu and psu[k] is not None:
                    data[k] = resolver.resolve(psu[k], throw=False)
            self.psu_obj.append(data)

        self.verbose = False

    def get_num_psus(self):
        return self.psu_num

    def _psu_refresh(self, index, pin_set):
        if index is None:
            return None

        #index from 1
        if index < 1 :
            return None
        if index > self.psu_num :
            return None

        obj = self.psu_obj[index - 1].get(pin_set, None)
        if obj:
            try:
                v = obj.get_value()
                if self.verbose:
                    name = self.psu[index - 1]['name']
                    print('{}: {}: {}'.format(name, obj.pathspec, v))
                return v
            except Exception as e:
                if self.verbose:
                    name = self.psu[index - 1]['name']
                    print('{}: {}: {}'.format(name, obj.pathspec , e))
        return None

    def get_psu_status(self, index):
        power = self._psu_refresh(index, 'power_ok')
        if power is not None and power == 0:
            return False

        psu_in = self._psu_refresh(index, 'power_in_ok')
        if psu_in is not None and psu_in == 0:
            return False

        fault = self._psu_refresh(index, 'fault')
        if fault is not None and fault == 1:
            return False

        return True

    def get_psu_presence(self, index):
        presence = self._psu_refresh(index,'presence')
        if presence is None:
            return False

        return presence == 1
