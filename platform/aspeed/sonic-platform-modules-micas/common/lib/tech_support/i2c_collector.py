#!/usr/bin/env python_nos
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd
from platform_tech_config import I2C_COLLECT


class I2CCollector(CollectorBase):
    name = 'i2c'

    def collect_i2c_detect_bus(self, bus):
        info = {}
        status, detect_info = run_cmd(f'i2cdetect -y {bus}')
        info[f'bus_{bus}_detect'] = detect_info
        return info

    def collect_i2c_detect(self, detect_bus):
        detect_info = {}
        for index, item in enumerate(detect_bus):
            start_bus = item["start_bus"]
            end_bus = item["end_bus"]
            if start_bus < 0 or end_bus < 0 or end_bus < start_bus:
                errmsg = f"detect_bus config error, index: {index}, start_bus: {start_bus}, end_bus: {end_bus}"
                logger.error(errmsg)
                bus_info = {}
                bus_info[f"detect_bus_config_{index}"] = errmsg
                detect_info.update(bus_info)
                continue
            for bus in range(start_bus, end_bus + 1):
                bus_info = self.collect_i2c_detect_bus(bus)
                detect_info.update(bus_info)
        return detect_info

    def collect(self):
        logger.info("Starting i2c collect...")
        info = {}
        status, bus_info = run_cmd('i2cdetect -l |sort -V', use_shell=True)
        info['i2c_detect_list'] = bus_info

        if I2C_COLLECT.get("i2c_detect", 0) == 1:
            detect_bus = I2C_COLLECT.get("detect_bus", [])
            bus_detect_info = self.collect_i2c_detect(detect_bus)
            info.update(bus_detect_info)
        logger.info("Finish i2c collect...")
        return info
