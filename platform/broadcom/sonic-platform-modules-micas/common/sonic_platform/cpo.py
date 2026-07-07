#!/usr/bin/python
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#############################################################################

try:
    import os
    import logging
    import math
    from logging.handlers import RotatingFileHandler
    from platform_intf import *
    from sonic_platform_base.sonic_xcvr.bailly_optoe_base import CpoOptoeBase, get_cpo_json_data
    from sonic_platform_base.sonic_xcvr.api.broadcom.bailly_rlm import BaillyApi
    from sonic_platform_base.sonic_xcvr.mem_maps.broadcom.bailly_rlm import BaillyMemMap
    from sonic_platform_base.sonic_xcvr.codes.broadcom.bailly_rlm import BaillyCodes
    from sonic_platform_base.sonic_xcvr.xcvr_eeprom import XcvrEeprom

except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error

OE_BANK_NUM = 8

LOG_TRACE_LEVEL = 0
LOG_DEBUG_LEVEL = 1
LOG_WARNING_LEVEL = 2
LOG_ERROR_LEVEL = 3

class CPO(CpoOptoeBase):
    def __init__(self, index, oe_id, oe_bank_id,  els_id, els_bank_id):
        super().__init__()
        self._port_id = index
        self._oe_id = oe_id
        self._oe_bank_id = oe_bank_id
        self._els_id = els_id
        self._els_bank_id= els_bank_id
        # need after _oe_bank_id init
        self._log_level_config = logging.ERROR
        self._setup_logging()
        self.remove_xcvr_api()
        self.get_xcvr_api()
        self._cpolog(LOG_DEBUG_LEVEL, "cpo__init__ {},{},{},  {},{} ".
            format(self._oe_id, self._port_id, self._oe_bank_id, self._els_id, self._els_bank_id))
        self.check_and_set_eeprom_bank_size()
        

    def get_xcvr_api(self):
        """
        Retrieves the XcvrApi associated with this SFP

        Returns:
            An object derived from XcvrApi that corresponds to the SFP
        """
        els_base_page = self.get_els_base_page()
        loc_bank = int(self._oe_bank_id % OE_BANK_NUM)
        self._cpolog(LOG_DEBUG_LEVEL, "get_xcvr_api bailly {},{},bank:{},{},  {},{} els_base_page:{}".
            format(self._oe_id, self._port_id, self._oe_bank_id, loc_bank, self._els_id, self._els_bank_id, els_base_page))
        oe_xcvr_eeprom = XcvrEeprom(self.read_eeprom, self.write_eeprom, BaillyMemMap(BaillyCodes, bank=loc_bank, base_page=els_base_page))
        self._xcvr_api = BaillyApi(oe_xcvr_eeprom)
        return self._xcvr_api
    
    def get_oe_eeprom_bank_size_path(self):
        cpo_bus = self.get_oes_config().get("oe_cmis_path", None)
        return cpo_bus + "max_bank_size" if cpo_bus is not None else None

    def get_eeprom_bank_size(self):
        try:
            bank_size_path = self.get_oe_eeprom_bank_size_path()
            with open(bank_size_path, 'r', encoding='utf-8') as f:
                content = f.read().strip()
            return int(content)
        except Exception as e:
            return None
    
    def set_eeprom_bank_size(self, value):
        try:
            bank_size_path = self.get_oe_eeprom_bank_size_path()
            with open(bank_size_path, 'w', encoding='utf-8') as f:
                f.write(str(value))
            self._cpolog(LOG_DEBUG_LEVEL, f"set bank_size: {self._port_id}, {value}")
            return True
        except Exception as e:
            self._cpolog(LOG_ERROR_LEVEL, f"set bank_size fail: {self._port_id}, {e}")
            return False

    def check_and_set_eeprom_bank_size(self):
        current_val = self.get_eeprom_bank_size()
        if current_val is None:
            self._cpolog(LOG_ERROR_LEVEL, f"bank_size is none: {self._port_id}")
            return False

        api = self.get_xcvr_api()
        api_size = api.get_max_supported_banks() 
        self._cpolog(LOG_DEBUG_LEVEL, f"max api_size is: {api_size}")
        size_advt = api_size 
        if current_val == size_advt:
            self._cpolog(LOG_DEBUG_LEVEL, f"bank_size is ok: {self._port_id}")
            return True
        else:
            self._cpolog(LOG_DEBUG_LEVEL, f"bank_size cur: {self._port_id}, {current_val}, set:{size_advt}")
            return self.set_eeprom_bank_size(size_advt)

    def get_eeprom_path(self):
        return self.get_oe_eeprom_path()

    def _setup_logging(self):
        log_dir = "/var/log/cpo"
        os.makedirs(log_dir, exist_ok=True)
        log_file = os.path.join(log_dir, "cpo_syslog.log")

        self.logger = logging.getLogger("cpo")
        self.logger.setLevel(self._log_level_config)
        
        self.logger.propagate = False

        if not self.logger.handlers:
            handler = RotatingFileHandler(
                log_file,
                maxBytes=10 * 1024 * 1024,
                backupCount=5,
                encoding='utf-8'
            )
            formatter = logging.Formatter(
                '[%(asctime)s] [%(levelname)s] %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S'
            )
            handler.setFormatter(formatter)
            self.logger.addHandler(handler)

    def _cpolog(self, log_level, msg):
        try:
            if log_level == LOG_DEBUG_LEVEL:
                self.logger.debug(msg)
            elif log_level == LOG_WARNING_LEVEL:
                self.logger.warning(msg)
            elif log_level == LOG_ERROR_LEVEL:
                self.logger.error(msg)
            else:
                self.logger.info(msg)
        except Exception as e:
            print(f"write_log_failed: {e}")
            import traceback
            traceback.print_exc()

    def get_port_laser_ids(self):
        ports_config = get_cpo_json_data().get("interfaces", None) 
        for eth_name, eth_info in ports_config.items():
            port_id = eth_info.get("index", 0).split(",")[0]
            if port_id == self._port_id:
                return eth_info.get("laser_ids", None)
        return None

    def get_transceiver_dom_real_value(self):
        info = super().get_transceiver_dom_real_value()
        if info is None:
            info = {}
        
        api = self.get_xcvr_api()
        if api is None:
            return info

        rlm_monitors = api.get_rlm_monitor_values()
        if rlm_monitors:
            info.update({
                key: value for key, value in rlm_monitors.items()
                if value is not None
            })

        try:
            els_laser_ids = self.get_port_laser_ids()
            if els_laser_ids is None:
                return info

            # add laser current info
            laser_current = api.get_rlm_laser_current()
            if not laser_current:
                self._cpolog(LOG_ERROR_LEVEL, f"get_rlm_laser_current error: {self._port_id}: {els_laser_ids}")
                return info

            laser_power = api.get_rlm_laser_power()
            if not laser_power:
                self._cpolog(LOG_ERROR_LEVEL, f"get_rlm_laser_power error: {self._port_id}: {els_laser_ids}")
                return info

            for idx in els_laser_ids:
                if idx < 0 or idx >= len(laser_current) or idx >= len(laser_power):
                    self._cpolog(LOG_ERROR_LEVEL, f"rlm_laser_ids idx error: {self._port_id}: {els_laser_ids}")
                    continue

                current_key = f"Laser{idx}CurrentMonitor"
                if current_key not in laser_current:
                    self._cpolog(LOG_ERROR_LEVEL, f"current key not found: {current_key}")
                    continue
                current_val = laser_current[current_key]
                if isinstance(current_val, dict):
                    current_val = current_val.get(current_key)
                if current_val is None:
                    self._cpolog(LOG_ERROR_LEVEL, f"current value not found: {current_key}")
                    continue
                current_str = f"{current_val} mA"

                power_key = f"Laser{idx}OpticalPowerMonitor"
                if power_key not in laser_power:
                    self._cpolog(LOG_ERROR_LEVEL, f"power key not found: {power_key}")
                    continue
                PmW = laser_power[power_key]
                if isinstance(PmW, dict):
                    PmW = PmW.get(power_key)
                if PmW is None:
                    self._cpolog(LOG_ERROR_LEVEL, f"power value not found: {power_key}")
                    continue
                if PmW > 0:
                    Pdb = 10 * math.log(PmW, 10)
                else:
                    Pdb = -40
                power_str = '%.03f mW %.03f dBm' % (PmW, Pdb)

                info.update({f"RLM{self._els_id}_Laser{idx}_current" : current_str})
                info.update({f"RLM{self._els_id}_Laser{idx}_power" : power_str})
        except Exception as e:
            self._cpolog(LOG_ERROR_LEVEL, f"get_rlm_laser_monitor error: {self._port_id}: {e}")

        self._cpolog(LOG_DEBUG_LEVEL, f"dom real info: {info}")
        return info

    def check_fiber_dirty(self):
        pass
    def check_calibration(self):
        pass
    def is_els_power_sufficient(self):
        pass
    def is_calibration_checked(self):
        pass
    def is_fiber_checked(self):
        pass
    def is_els_tx_on(self):
        pass
    def is_els_tx_enabled(self):
        pass

