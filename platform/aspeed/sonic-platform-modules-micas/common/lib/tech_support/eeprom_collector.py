#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd

class EepromCollector(CollectorBase):
    name = "eeprom"


    def collect_platform_e2(self):
        result = {}
        logger.info("Starting collect platform_e2 info...")
        status, content = run_cmd(f'platform_e2.py all', timeout=10)
        result[f'eeprom_all_info'] = content
        logger.info("Finish collect platform_e2 info...")
        return result

    def collect(self):
        result = {}

        logger.info("Starting eeprom collect...")
        # collect platform_e2 info
        tmp_info = self.collect_platform_e2()
        result.update(tmp_info)

        logger.info("Finish eeprom collect...")
        return result