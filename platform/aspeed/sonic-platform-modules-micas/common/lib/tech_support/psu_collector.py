#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd, collect_driver_info

class PSUCollector(CollectorBase):
    name = "psu"

    def collect_psu_pmbus(self):
        # Traverse wb_pmbus I2C devices sysfs
        root_dir = "/sys/bus/i2c/drivers/wb_pmbus"
        result = collect_driver_info(root_dir)
        return result

    def collect_psu_dfx_info(self):
        # Read /proc/psu/*/dfx_info
        result = {}
        proc_root = "/proc/psu"

        logger.info("Starting collect psu dfx_info...")

        if not os.path.exists(proc_root):
            return {"proc_psu_error": f"{proc_root} not exists"}

        for dev in sorted(os.listdir(proc_root)):
            bb = os.path.join(proc_root, dev, "dfx_info")
            key = f"proc_psu/{dev}/dfx_info"
            logger.debug(f"collect: {bb}")
            if not os.path.exists(bb):
                result[key] = f"{bb} not exists"
                continue
            try:
                status, content = run_cmd(f'cat {bb}')
            except Exception as e:
                content = f"<read error: {e}>"
                logger.error(content)
            result[key] = content

        logger.info("Finish collect psu dfx_info...")

        if not result:
            return {"proc_psu_dev_error": f"proc psu device not exists"}
        return result

    def collect_psu_eeprom(self):
        result = {}
        logger.info("Starting collect psu eeprom...")
        status, content = run_cmd(f'platform_e2.py psu all', timeout=3)
        result[f'psu_eeprom'] = content
        logger.info("Finish collect psu eeprom...")
        return result

    def collect_psu_hal(self):
        result = {}
        logger.info("Starting collect psu hal info...")
        status, content = run_cmd(f'hal_pltfm.py psu get_psu_power_status', timeout=3)
        result[f'psu_hal_power_status'] = content
        logger.info("Finish collect psu hal info...")
        return result

    def collect(self):
        result = {}
        logger.info("Starting psu collect...")
        # collect psu pmbus info
        tmp_info = self.collect_psu_pmbus()
        result.update(tmp_info)

        # collect psu dfx info
        tmp_info = self.collect_psu_dfx_info()
        result.update(tmp_info)

        # collect psu eeprom info
        tmp_info = self.collect_psu_eeprom()
        result.update(tmp_info)

        # collect psu hal info
        tmp_info = self.collect_psu_hal()
        result.update(tmp_info)

        logger.info("Finish psu collect...")
        return result