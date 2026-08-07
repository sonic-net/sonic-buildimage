#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd
from public.platform_common_config import (
    SERVICE_LIST,
)

class ServiceCollector(CollectorBase):
    name = "service"

    def collect_service_single(self, service_name):
        result = {}
        logger.info(f"Starting collect {service_name} info...")

        # enable status
        status, content = run_cmd(f'systemctl is-enabled {service_name}')
        result[f"{service_name}_enable"] = content

        # status
        status, content = run_cmd(f'systemctl status {service_name}')
        result[f"{service_name}_status"] = content

        # config
        status, content = run_cmd(f'systemctl cat {service_name}')
        result[f"{service_name}_config"] = content

        # config
        status, content = run_cmd(f'systemctl list-dependencies {service_name}')
        result[f"{service_name}_dep"] = content

        logger.info(f"Finish collect {service_name} info...")
        return result

    def collect_service_info(self):
        result = {}
        for service in SERVICE_LIST:
            tmp_info = self.collect_service_single(service)
            result.update(tmp_info)
        return result

    def collect(self):
        result = {}

        logger.info("Starting service collect...")
        # collect vr driver info
        tmp_info = self.collect_service_info()
        result.update(tmp_info)

        logger.info("Finish service collect...")
        return result