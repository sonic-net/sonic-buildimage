#!/usr/bin/env python_nos
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd


class PciCollector(CollectorBase):
    name = 'pci'

    def collect(self):
        logger.info("Starting PCI collect...")
        info = {}

        status, pci_tree = run_cmd("lspci -tv")
        info['pci_tree'] = pci_tree

        status, all_info = run_cmd("lspci -vvvvxxxx", timeout=5)
        info['all_pci_info'] = all_info

        logger.info("Finish PCI collect...")
        return info

