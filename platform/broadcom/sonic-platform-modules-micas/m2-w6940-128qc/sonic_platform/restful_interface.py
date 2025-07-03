from restful_util.restful_api import RestfulApiClient
from platform_util import exec_os_cmd, get_value, dev_file_read
from plat_hal.baseutil import baseutil
import time
import sys
PY3K = sys.version_info >= (3, 0)
if PY3K:
    from time import monotonic as _time
else:
    from monotonic import monotonic as _time

PSU_NOT_PRESENCE = 0x01
PSU_POWER_LOSS = 0x02
PSU_FAN_FAULT = 0x04
PSU_VOL_FAULT = 0x08
PSU_CUR_FAULT = 0x10
PSU_PWR_FAULT = 0x20
PSU_TEMP_FAULT = 0x40

PSU_ERR_VALUE = -999999


def RestfulInfoUpdate():
    def decorator(func):
        def wrapper(self, *args, **kwargs):
            self.update_restful_info(func.__name__)
            return func(self, *args, **kwargs)
        return wrapper
    return decorator


class RestfulApi(RestfulApiClient):

    func_map = {
        "get_psu_total_number": "psu",
        "get_psu_info": "psu",
        "get_fan_total_number": "fan",
        "get_fan_rotor_number": "fan",
        "get_fan_info": "fan",
        "get_all_dcdc_sensors": "sensor",
        "get_dcdc_sensor_by_id": "sensor",
        "get_all_temp_sensors": "temp_sensor",
        "get_temp_sensor_by_id": "temp_sensor",
        "get_all_vol_sensors": "vol_sensor",
        "get_vol_sensor_by_id": "vol_sensor",
        "get_all_cur_sensors": "cur_sensor",
        "get_cur_sensor_by_id": "cur_sensor",
        "get_cpld_total_number": "fw",
        "get_cpld_by_id": "fw",
        "get_bmc_by_id": "fw",
        "get_bmc_total_number": "fw",
        "get_sysled": "led",
    }

    def __init__(self, default_gap=3):
        self.gap = default_gap
        self.config = baseutil.get_config()
        self.eeprom_config = self.config.get("onie_e2", None)
        self.reboot_cause_config = self.config.get("reboot_cause", None)
        self.fpga_config = self.config.get("fpga_config", {})
        self.bios_config = self.config.get("bios_config", {})
        self.sw_config = self.config.get("switch_config", {})
        self.cpld_config = self.config.get("cpld_config", {})
        self.bmc_config = self.config.get("bmc_config", {})
        self.fan_config = self.config.get("fan_config", {})
        self.psu_config = self.config.get("psu_config", {})
        self.thermal_config = self.config.get("thermal_config", {})
        self.restful_ready = 0
        self.dcdc_sensors = None
        self.fans = None
        self.psus = None
        self.temp_sensors = None
        self.vol_sensors = None
        self.cur_sensors = None
        self.cplds = None
        self.bmc = None
        self.sysled = None
        self.temp_filter = {
            "temp0": "INLET_TEMP_PDB",
            "temp1": "INLET_TEMP_LC",
            "temp2": "OUTLET_2_TEMP",
            "temp3": "OUTLET_1_TEMP",
            "temp4": "ASIC_TEMP",
            "temp5": "CPU_TEMP",
            "temp6": "PSU1_TEMP",
            "temp7": "PSU2_TEMP",
            "temp8": "PSU3_TEMP",
            "temp9": "PSU4_TEMP",
        }
        self.restful_update_map = {
            "psu": {
                "update_time": 0,
                "update_func": self.update_psus,
                "url": self.PsusUrl
            },
            "fan": {
                "update_time": 0,
                "update_func": self.update_fans,
                "url": self.FansUrl
            },
            "fw": {
                "update_time": 0,
                "update_func": self.update_firmwares,
                "url": self.FirmwaresUrl
            },
            "sensor": {
                "update_time": 0,
                "update_func": self.update_sensors,
                "url": self.SensorsUrl
            },
            "temp_sensor": {
                "update_time": 0,
                "update_func": self.update_temp_sensors,
                "url": self.TempSensorsUrl
            },
            "vol_sensor": {
                "update_time": 0,
                "update_func": self.update_vol_sensors,
                "url": self.VolSensorsUrl
            },
            "cur_sensor": {
                "update_time": 0,
                "update_func": self.update_cur_sensors,
                "url": self.CurSensorsUrl
            },
            "led": {
                "update_time": 0,
                "update_func": self.update_sysled,
                "url": self.LEDsUrl
            },
        }
        self.restful_err_str = ("ACCESS FAILED", "NA", "N/A")

    def restful_check_ready(self):
        if self.restful_ready == 1:
            return True
        retry = 60
        while retry >= 0:
            tmp = self.get_request(self.HostnameUrl)
            if tmp is None:
                time.sleep(5)
                continue
            ret = tmp.get('message', 'Not OK')
            code = tmp.get('code', 0)
            if ret == 'OK' and code == 200:
                self.restful_ready = 1
                return True
        return False

    def update_restful_info(self, func_name):
        tmp = self.restful_update_map.get(self.func_map.get(func_name, None))
        if tmp is not None:
            last_time = tmp.get("update_time", 0)
            local_time = _time()
            func = tmp.get("update_func", None)
            url = tmp.get("url", None)
            if func is not None and local_time - last_time > self.gap:
                ret, data = self.restful_request(url)
                if ret:
                    func(data)
                    tmp["update_time"] = _time()

    def restful_request(self, url):
        try:
            tmp = self.get_request(url)
            if not tmp:
                return False, None
            ret = tmp.get('message', 'Not OK')
            code = tmp.get('code', 0)
            if ret == 'OK' and code == 200:
                data = tmp.get('data', None)
                return True, data
            return False, None
        except Exception as e:
            return False, None

    def update_firmwares(self, val):
        try:
            self.cplds = val.get('cpld', None)
            self.bmc = val.get('bmc', None)
        except Exception as e:
            self.cplds = None
            self.bmc = None

    def update_sysled(self, val):
        try:
            self.sysled = val.get('sysled', None)
        except Exception as e:
            self.sysled = None

    def update_psus(self, val):
        try:
            self.psus = val.get('psu', None)
        except Exception as e:
            self.psus = None

    def update_fans(self, val):
        try:
            self.fans = val.get('fan', None)
        except Exception as e:
            self.fans = None

    def update_sensors(self, val):
        try:
            cur_sensors = val.get('cur_sensor', None)
            vol_sensors = val.get('vol_sensor', None)
            temp_sensors = val.get('temp_sensor', None)
            self.dcdc_sensors = {}
            self.temp_sensors = {}
            if cur_sensors is not None:
                for name, data in cur_sensors.items():
                    if isinstance(data, dict):
                        self.dcdc_sensors[name] = data
            if vol_sensors is not None:
                for name, data in vol_sensors.items():
                    if isinstance(data, dict):
                        self.dcdc_sensors[name] = data
            if temp_sensors is not None:
                for name, data in temp_sensors.items():
                    if isinstance(data, dict):
                        if name in self.temp_filter:
                            self.temp_sensors[name] = data
                            self.temp_sensors[name]["alias"] = self.temp_filter[name]
        except Exception as e:
            self.dcdc_sensors = None
            self.temp_sensors = None

    def update_temp_sensors(self, val):
        try:
            temp_sensors = val.get('temp_sensor', None)
            self.temp_sensors = {}
            if temp_sensors is not None:
                for name, data in temp_sensors.items():
                    if isinstance(data, dict):
                        if name in self.temp_filter:
                            self.temp_sensors[name] = data
                            self.temp_sensors[name]["alias"] = self.temp_filter[name]
        except Exception as e:
            self.temp_sensors = None

    def update_vol_sensors(self, val):
        try:
            vol_sensors = val.get('vol_sensor', None)
            self.vol_sensors = {}
            if vol_sensors is not None:
                for name, data in vol_sensors.items():
                    if isinstance(data, dict):
                        self.vol_sensors[name] = data
        except Exception as e:
            self.vol_sensors = None

    def update_cur_sensors(self, val):
        try:
            cur_sensors = val.get('cur_sensor', None)
            self.cur_sensors = {}
            if cur_sensors is not None:
                for name, data in cur_sensors.items():
                    if isinstance(data, dict):
                        self.cur_sensors[name] = data
        except Exception as e:
            self.cur_sensors = None

    # Interface for Psu
    def get_psu_total_number(self):
        psu_num = self.psu_config.get("number", 0)
        return psu_num


    @RestfulInfoUpdate()
    def get_psu_info(self, psu_name):
        if self.psus is None:
            return None
        return self.psus.get(psu_name)

    # Interface for Fan
    def get_fan_total_number(self):
        fan_num = self.fan_config.get("num_fantrays", 0)
        return fan_num

    def get_fan_rotor_number(self, fan_name):
        rotor_number = self.fan_config.get("num_fans_pertray", 0)
        return rotor_number

    @RestfulInfoUpdate()
    def get_fan_info(self, fan_name):
        if self.fans is None:
            return None
        return self.fans.get(fan_name)

    @RestfulInfoUpdate()
    def get_all_dcdc_sensors(self):
        return self.dcdc_sensors

    # Interface for dcdc
    @RestfulInfoUpdate()
    def get_dcdc_sensor_by_id(self, dcdc_id):
        if not self.dcdc_sensors:
            return None
        return self.dcdc_sensors.get(dcdc_id, None)

    def get_thermal_total_number(self):
        thermal_num = self.thermal_config.get("number", 0)
        return thermal_num

    @RestfulInfoUpdate()
    def get_all_temp_sensors(self):
        return self.temp_sensors

    @RestfulInfoUpdate()
    def get_temp_sensor_by_id(self, temp_id):
        if not self.temp_sensors:
            return None
        return self.temp_sensors.get(temp_id, None)

    @RestfulInfoUpdate()
    def get_all_vol_sensors(self):
        return self.vol_sensors

    @RestfulInfoUpdate()
    def get_vol_sensor_by_id(self, vol_id):
        if not self.vol_sensors:
            return None
        return self.vol_sensors.get(vol_id, None)

    @RestfulInfoUpdate()
    def get_all_cur_sensors(self):
        return self.cur_sensors

    @RestfulInfoUpdate()
    def get_cur_sensor_by_id(self, cur_id):
        if not self.cur_sensors:
            return None
        return self.cur_sensors.get(cur_id, None)

    def get_cpld_total_number(self):
        cpld_num = self.cpld_config.get("number", 0)
        return cpld_num

    def get_fw_by_id(self, fw_name):
        if "cpld" in fw_name:
            return self.get_cpld_by_id(fw_name)
        elif "bmc" in fw_name:
            return self.get_bmc_by_id(fw_name)
        elif "switch" in fw_name:
            return self.get_switch_by_id(fw_name)
        elif "bios" in fw_name:
            return self.get_bios_by_id(fw_name)
        elif "fpga" in fw_name:
            return self.get_fpga_by_id(fw_name)

    @RestfulInfoUpdate()
    def get_cpld_by_id(self, fw_type):
        if self.cplds is not None:
            cpld = self.cplds.get(fw_type, None)
            return cpld
        return None

    @RestfulInfoUpdate()
    def get_sysled(self):
        return self.sysled

    def _get_value(self, config):
        if config["gettype"] == "cmd":
            ret, output = exec_os_cmd(config["cmd"])
            return output

    def get_fpga_total_number(self):
        fpga_num = self.fpga_config.get("number", 0)
        return fpga_num

    def get_fpga_reg(self, config):
        if config is None:
            return "N/A"
        ret, tmp = dev_file_read(config["dev_path"], config["offset"], config["len"])
        if ret != True:
            return "N/A"
        reg = ""
        for index in range(len(tmp) - 1, -1, -1):
            val = hex(tmp[index]).replace("0x", "")
            reg = reg + val
        return reg

    def get_fpga_by_id(self, fw_type):
        fw_dict = {}
        fpgas = self.fpga_config.get("fpgas", [])
        for i in fpgas:
            if fw_type == i["name"]:
                fw_dict['alias'] = i.get('alias', None)
                fw_dict['board_version'] = self.get_fpga_reg(i.get("board_version", None))
                fw_dict['firmware_version'] = self.get_fpga_reg(i.get("firmware_version", None))
                fw_dict['type'] = i.get('type', None)
                break
        return fw_dict

    def get_bios_by_id(self, fw_type):
        fw_dict = {}
        bios = self.bios_config.get("bios", [])
        for i in bios:
            if fw_type == i["name"]:
                fw_dict['alias'] = i.get('alias', None)
                fw_dict['board_version'] = self._get_value(i.get("board_version", None))
                fw_dict['firmware_version'] = self._get_value(i.get("firmware_version", None))
                fw_dict['type'] = self._get_value(i.get("type", None))
                break
        return fw_dict

    def get_switch_by_id(self, fw_type):
        fw_dict = {}
        sw = self.sw_config.get("switchs", [])
        for i in sw:
            if fw_type == i["name"]:
                for cmd in i.get("init_cmd", []):
                    exec_os_cmd(cmd)
                fw_dict['alias'] = i.get('alias', None)
                fw_dict['board_version'] = self._get_value(i.get("board_version", None)).split(" ")[0]
                fw_dict['firmware_version'] = self._get_value(i.get("firmware_version", None)).split(" ")[0]
                fw_dict['type'] = i.get('type', None)
                for cmd in i.get("deinit_cmd", []):
                    exec_os_cmd(cmd)
                break
        return fw_dict

    @RestfulInfoUpdate()
    def get_bmc_by_id(self, fw_type):
        fw_dict = {}
        if self.bmc is not None:
            bmc = self.bmc.get(fw_type)
            fw_dict['board_version'] = bmc.get('board_version', None)
            fw_dict['alias'] = bmc.get('alias', None)
            fw_dict['firmware_version'] = bmc.get('firmware_version', None)
            fw_dict['type'] = bmc.get('type', None)
        return fw_dict

    def get_bmc_total_number(self):
        bmc_num = self.bmc_config.get("number", 0)
        return bmc_num

    def get_bios_total_number(self):
        num = self.bios_config.get("number", 0)
        return num

    def get_sw_total_number(self):
        num = self.sw_config.get("number", 0)
        return num

    def get_onie_e2_path(self, e2_name):
        if self.eeprom_config is None:
            return "/sys/bus/i2c/devices/1-0056/eeprom"
        return self.eeprom_config.get("e2loc").get("loc")

    def get_cpu_reboot_cause(self):
        if self.reboot_cause_config is None:
            return 0
        check_point = self.reboot_cause_config.get('check_point', None)
        if check_point is not None:
            ret, val = get_value(check_point)
            if ret == True:
                return val
        return 0
