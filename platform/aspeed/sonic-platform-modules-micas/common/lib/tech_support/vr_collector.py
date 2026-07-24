#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd, collect_driver_info

class VRCollector(CollectorBase):
    name = "vr"

    def collect_wb_ucd9000(self):
        root_dir = "/sys/bus/i2c/drivers/wb_ucd9000"
        binary_file_list = ["fault_record"]
        result = collect_driver_info(root_dir, binary_file_list)
        return result

    def collect_wb_ucd9081(self):
        root_dir = "/sys/bus/i2c/drivers/wb_ucd9081"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_ina3221(self):
        root_dir = "/sys/bus/i2c/drivers/wb_ina3221"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_xdpe12284(self):
        root_dir = "/sys/bus/i2c/drivers/wb_xdpe12284"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_xdpe132g5c(self):
        root_dir = "/sys/bus/i2c/drivers/wb_xdpe132g5c"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_xdpe132g5c_pmbus(self):
        root_dir = "/sys/bus/i2c/drivers/wb_xdpe132g5c_pmbus"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_isl68137(self):
        root_dir = "/sys/bus/i2c/drivers/wb_isl68137"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_tps53622(self):
        root_dir = "/sys/bus/i2c/drivers/wb_tps53622"
        result = collect_driver_info(root_dir)
        return result

    def collect_wb_jwh6384(self):
        root_dir = "/sys/bus/i2c/drivers/wb_jwh6384"
        result = collect_driver_info(root_dir)
        return result

    def collect_vr_driver(self):
        result = {}
        logger.info("Starting collect vr driver info...")

        tmp_info = self.collect_wb_ucd9000()
        result.update(tmp_info)

        tmp_info = self.collect_wb_ucd9081()
        result.update(tmp_info)

        tmp_info = self.collect_wb_ina3221()
        result.update(tmp_info)

        tmp_info = self.collect_wb_xdpe12284()
        result.update(tmp_info)

        tmp_info = self.collect_wb_xdpe132g5c()
        result.update(tmp_info)

        tmp_info = self.collect_wb_xdpe132g5c_pmbus()
        result.update(tmp_info)

        tmp_info = self.collect_wb_isl68137()
        result.update(tmp_info)

        tmp_info = self.collect_wb_tps53622()
        result.update(tmp_info)

        tmp_info = self.collect_wb_jwh6384()
        result.update(tmp_info)

        logger.info("Finish collect vr driver info...")
        return result


    def collect_vr_hal(self):
        result = {}
        logger.info("Starting collect dcdc info...")
        status, content = run_cmd(f'hal_pltfm.py dcdc get_dcdc_all_info', timeout=5)
        result[f'dcdc_all_info'] = content
        logger.info("Finish collect dcdc info...")
        return result

    def collect(self):
        result = {}

        logger.info("Starting vr collect...")
        # collect vr driver info
        tmp_info = self.collect_vr_driver()
        result.update(tmp_info)

        # collect vr hal info
        tmp_info = self.collect_vr_hal()
        result.update(tmp_info)

        logger.info("Finish vr collect...")
        return result