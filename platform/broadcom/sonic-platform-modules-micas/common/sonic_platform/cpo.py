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
    import fcntl
    import logging
    import os
    import time
    from contextlib import contextmanager
    from logging.handlers import RotatingFileHandler

    from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoBase
    from sonic_platform_base.sonic_xcvr.cpo.cpo_optoe_base import OptoeOeBase, OptoeElsfpBase
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_xcvr.fields import elsfp_consts
    from sonic_platform_base.sonic_xcvr.api.xcvr_api import XcvrApi
    from sonic_platform_base.sonic_xcvr.optoe_eeprom_rw import (
        SFP_OPTOE_PAGE_SELECT_OFFSET, SFP_OPTOE_UPPER_PAGE0_OFFSET, SFP_OPTOE_PAGE_SIZE)
except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error


LOG_DEBUG_LEVEL = 1
LOG_WARNING_LEVEL = 2
LOG_ERROR_LEVEL = 3



EEPROM_LOCK_TIMEOUT = 2.0
EEPROM_LOCK_RETRY = 0.001
EEPROM_ACCESS_DELAY = 0.01


class _BaillyEepromAccessMixin:
    """Serialize joint OE/ELS MCU access and retain its settling interval."""

    @contextmanager
    def _locked_eeprom(self, mode):
        # Lock the EEPROM node itself so all endpoints/processes sharing the
        # MCU coordinate, including callers of the legacy CPO implementation.
        with open(self.get_eeprom_path(), mode=mode, buffering=0) as fd:
            deadline = time.monotonic() + EEPROM_LOCK_TIMEOUT
            while True:
                try:
                    fcntl.flock(fd.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                    break
                except BlockingIOError:
                    if time.monotonic() >= deadline:
                        raise TimeoutError("EEPROM lock timeout")
                    time.sleep(EEPROM_LOCK_RETRY)
            try:
                yield fd
            finally:
                try:
                    # Keep the lock until the MCU can accept its next access.
                    time.sleep(EEPROM_ACCESS_DELAY)
                finally:
                    fcntl.flock(fd.fileno(), fcntl.LOCK_UN)

    def read_eeprom(self, offset, num_bytes):
        if not self.get_eeprom_path():
            return None
        try:
            with self._locked_eeprom('rb') as fd:
                if SFP_OPTOE_UPPER_PAGE0_OFFSET <= offset < (
                        SFP_OPTOE_UPPER_PAGE0_OFFSET + SFP_OPTOE_PAGE_SIZE):
                    # Preserve Optoe's page-0 recovery without recursively
                    # acquiring the same MCU lock through read/write_eeprom.
                    fd.seek(SFP_OPTOE_PAGE_SELECT_OFFSET)
                    page = fd.read(1)
                    if len(page) != 1:
                        return None
                    time.sleep(EEPROM_ACCESS_DELAY)
                    if page != b'\x00':
                        with open(self.get_eeprom_path(), 'r+b', buffering=0) as selector:
                            selector.seek(SFP_OPTOE_PAGE_SELECT_OFFSET)
                            if selector.write(b'\x00') != 1:
                                return None
                        time.sleep(EEPROM_ACCESS_DELAY)
                fd.seek(offset)
                data = fd.read(num_bytes)
            return bytearray(data) if len(data) == num_bytes else None
        except OSError as exc:
            logging.getLogger("cpo").error("%s EEPROM read at %s failed: %s", self.get_name(), offset, exc)
            return None

    def write_eeprom(self, offset, num_bytes, write_buffer):
        if not self.get_eeprom_path() or len(write_buffer) < num_bytes:
            return False
        try:
            with self._locked_eeprom('r+b') as fd:
                fd.seek(offset)
                written = fd.write(write_buffer[:num_bytes])
            return written == num_bytes
        except OSError as exc:
            logging.getLogger("cpo").error("%s EEPROM write at %s failed: %s", self.get_name(), offset, exc)
            return False


class Oe(_BaillyEepromAccessMixin, OptoeOeBase):
    def __init__(self, oe_id, oe_config, hardware_info, bank=0, base_page=0):
        self.oe_id = int(oe_id)
        cmis_path = oe_config.get("i2c_path")
        self._eeprom_path = os.path.join(cmis_path, "eeprom") if cmis_path else ""
        self.base_page = int(base_page)
        super().__init__(hardware_info, bank=bank)

    def get_eeprom_path(self) -> str:
        return self._eeprom_path

    def get_lpmode(self) -> bool:
        lpmode = self.get_api().get_lpmode()
        if not isinstance(lpmode, bool):
            raise OSError("OE low-power status is unavailable")
        return lpmode

    def set_lpmode(self, lpmode):
        return self.get_api().set_lpmode(lpmode)

    def reset(self):
        return self.get_api().reset()

    def get_name(self):
        return "OE{}".format(self.oe_id)

    def get_presence(self):
        return True

    def is_replaceable(self):
        return False


class Elsfp(_BaillyEepromAccessMixin, OptoeElsfpBase):
    """Bailly ELS endpoint sharing its OE MCU lock and access pacing."""

    def __init__(self, els_id, els_config, eeprom_path, hardware_info, bank=0):
        self.els_id = int(els_id)
        self._presence_config = dict(els_config.get("els_presence") or {})
        self._eeprom_path = eeprom_path or ""
        self.base_page = int(els_config.get("base_page", 0))
        super().__init__(hardware_info, bank=bank)

    def get_eeprom_path(self) -> str:
        return self._eeprom_path

    def get_lpmode(self) -> bool:
        api = self.get_api()
        status = api.get_rlm_status()
        if not isinstance(status, dict):
            raise OSError("ELS low-power status is unavailable")
        power_mode = status.get("els_module_low_power_state")
        power_codes = api.xcvr_eeprom.mem_map.codes.POWER_MODE
        if power_mode == power_codes[1]:
            return True
        if power_mode == power_codes[0]:
            return False
        raise OSError("ELS low-power status is unavailable: {!r}".format(power_mode))

    def get_name(self):
        return "ELS{}".format(self.els_id)

    def get_presence(self):
        config = self._presence_config
        try:
            path = config.get("presence_file")
            offset = int(str(config.get("presence_offset", "0")), 16)
            size = int(config.get("presence_len", 0))
            bit = int(config.get("presence_bit", 0))
            present_value = int(config.get("presence_value", 1))
            if not path or size <= 0:
                return False
            with open(path, mode='rb', buffering=0) as fd:
                fd.seek(offset)
                raw = fd.read(size)
            if len(raw) != size:
                return False
            value = int.from_bytes(raw, byteorder='little')
            return ((value >> bit) & 1) == present_value
        except (OSError, TypeError, ValueError):
            return False

    def is_replaceable(self):
        return True


class Cpo(CpoBase, SfpOptoeBase):
    """Community CPO object with the transceiver methods used by xcvrd.

    CPO/SFP transceiver methods use the OE API; RLM monitors use the ELS API.
    No legacy CPO object or dynamic attribute forwarding is needed.
    Virtual-module controls retain joint-mode OE-controller behavior.
    """

    def __init__(self, port_id, name, oe, elsfp, laser_ids=None):
        self._port_id = int(port_id)
        self._name = name
        self._els_id = elsfp.els_id
        self._laser_ids = list(laser_ids or [])
        CpoBase.__init__(self, oe.hardware_id, oe, elsfp)
        SfpOptoeBase.__init__(self, bank=oe.bank)
        self._log_level_config = logging.ERROR
        self._setup_logging()

    def get_name(self):
        return self._name

    def get_presence(self):
        return bool(self.elsfp.get_presence())

    def get_position_in_parent(self):
        return self._port_id

    def is_replaceable(self):
        return False

    def get_eeprom_path(self):
        return self.oe.get_eeprom_path()

    def read_eeprom(self, offset, num_bytes):
        return self.oe.read_eeprom(offset, num_bytes)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        return self.oe.write_eeprom(offset, num_bytes, write_buffer)

    def get_reset_status(self):
        return False

    def reset(self):
        return SfpOptoeBase.reset(self)

    def get_lpmode(self):
        return SfpOptoeBase.get_lpmode(self)

    def set_lpmode(self, lpmode):
        return SfpOptoeBase.set_lpmode(self, lpmode)

    def get_tx_disable(self):
        return SfpOptoeBase.get_tx_disable(self)

    def tx_disable(self, tx_disable):
        return SfpOptoeBase.tx_disable(self, tx_disable)

    def get_port_laser_ids(self):
        return self._laser_ids

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

    def get_transceiver_dom_real_value(self):
        info = SfpOptoeBase.get_transceiver_dom_real_value(self)
        if info is None:
            info = {}

        try:
            api = self.elsfp.get_api()
            monitors = api.get_non_banked_elsfp_dom_real_value()
            if monitors is not None:
                for source, target in (("temperature", "els_temperature"),
                                       ("voltage", "els_voltage"), ("tec_current", "rlm_tec_current")):
                    if monitors.get(source) is not None:
                        info[target] = monitors[source]

            laser_ids = self.get_port_laser_ids()
            if not laser_ids:
                return info
            currents = api.get_per_lane_bias_current_monitor()
            powers = api.get_per_lane_opt_power_monitor()
            if currents is None or powers is None:
                self._cpolog(LOG_ERROR_LEVEL, "ELS laser monitor read failed: {}".format(self._port_id))
                return info

            for laser in laser_ids:
                current_key = "{}{}".format(elsfp_consts.BIAS_CURRENT_MONITOR_FIELD, laser + 1)
                power_key = "{}{}".format(elsfp_consts.OPT_POWER_MONITOR_FIELD, laser + 1)
                current = currents.get(current_key)
                power_mw = powers.get(power_key)
                if current is None or power_mw is None:
                    self._cpolog(LOG_ERROR_LEVEL, "ELS laser {} monitor unavailable: {}".format(laser, self._port_id))
                    continue
                current_ma = round(api.amps_to_ma(current), 3)
                power_dbm = XcvrApi.mw_to_dbm(power_mw)
                info["RLM{}_Laser{}_current".format(self._els_id, laser)] = "{} mA".format(current_ma)
                info["RLM{}_Laser{}_power".format(self._els_id, laser)] = "%.03f dBm" % power_dbm
        except Exception as exc:
            self._cpolog(LOG_ERROR_LEVEL, "get ELS laser monitors failed: {}: {}".format(self._port_id, exc))

        self._cpolog(LOG_DEBUG_LEVEL, "dom real info: {}".format(info))
        return info
