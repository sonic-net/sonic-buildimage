#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd, collect_driver_info

class TempCollector(CollectorBase):
    name = "temp"

    def collect_ct7318(self):
        root_dir = "/sys/bus/i2c/drivers/ct7318"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_lm75(self):
        root_dir = "/sys/bus/i2c/drivers/wb_lm75"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_tmp401(self):
        root_dir = "/sys/bus/i2c/drivers/wb_tmp401"
        result = collect_driver_info(root_dir)
        return result

    def collect_temp_driver(self):
        result = {}

        logger.info("Starting collect temp driver info...")
        tmp_info = self.collect_ct7318()
        result.update(tmp_info)

        tmp_info = self.collect_wb_lm75()
        result.update(tmp_info)

        tmp_info = self.collect_wb_tmp401()
        result.update(tmp_info)

        logger.info("Finish collect temp driver info...")
        return result

    def collect_temp_hal(self):
        result = {}
        logger.info("Starting collect temp hal info...")
        status, content = run_cmd(f'hal_pltfm.py temp get_temp_info', timeout=3)
        result[f'dcdc_all_info'] = content
        logger.info("Finish collect temp hal info...")
        return result

    def collect_ddr_temp(self):
        result = {}
        logger.info("Starting collect ddr temp info...")

        ddr_temp_files = [
            "mem_temp_1",
            "mem_temp_2",
            "mem_temp_3",
            "mem_temp_4",
        ]
        root_dir = "/sys/bus/pci/drivers/wb_spd"
        if not os.path.exists(root_dir):
            logger.info(f"{root_dir} not exists, skip")
            return result

        for entry in os.listdir(root_dir):
            full_path = os.path.join(root_dir, entry)

            if not os.path.islink(full_path) or entry == "module":
                continue

            dev_path = os.path.realpath(full_path)

            hwmon_root = os.path.join(dev_path, "hwmon")
            if not os.path.isdir(hwmon_root):
                continue

            for hwmon in os.listdir(hwmon_root):
                hwmon_path = os.path.join(hwmon_root, hwmon)
                if not os.path.isdir(hwmon_path):
                    continue

                for index, name in enumerate(ddr_temp_files):
                    key = "ddr_temp_slot%d" % index
                    file_path = os.path.join(hwmon_path, name)
                    if os.path.isfile(file_path):
                        status, content = run_cmd(f'cat {file_path}')
                    else:
                        content = f"{file_path} <not found>"
                    result[key] = content

        logger.info("Finish collect ddr temp info...")
        return result

    def collect(self):
        result = {}

        logger.info("Starting temp collect...")
        # collect vr driver info
        tmp_info = self.collect_temp_driver()
        result.update(tmp_info)

        # collect ddr temp info
        tmp_info = self.collect_ddr_temp()
        result.update(tmp_info)

        # collect vr hal info
        tmp_info = self.collect_temp_hal()
        result.update(tmp_info)

        logger.info("Finish temp collect...")
        return result
