#!/usr/bin/env python_nos
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd


class NicCollector(CollectorBase):
    name = 'nic'

    def collect(self):
        logger.info("Starting nic collect...")
        info = {}

        # list interfaces
        status, interfaces = run_cmd("ls /sys/class/net")
        info['interfaces'] = interfaces

        # show link status for each interface
        links = {}
        for ifc in info['interfaces'].split():
            status, operstate = run_cmd(f"cat /sys/class/net/{ifc}/operstate")
            links[ifc] = operstate
            if 'eth' in ifc:
                status, ethtool_info = run_cmd(f"ethtool {ifc}")
                status, ethtool_i_info = run_cmd(f"ethtool -i {ifc}")
                info[ifc + '_ethtool_info'] = ethtool_info + '\n' + ethtool_i_info

        info['operstate'] = links
        status, ifconfig = run_cmd("ifconfig -a")

        info['ifconfig'] = ifconfig

        status, ip_addr = run_cmd("ip addr show")
        info['ip_addr'] = ip_addr

        status, ip_route = run_cmd("ip route")
        info['ip_route'] = ip_route

        logger.info("Finish nic collect...")
        return info
