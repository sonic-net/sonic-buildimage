#!/usr/bin/env python_nos
import os
import glob
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd

PHY_COLL_INFO = {
    "link_status" : {
        "decode" : {
            "1" : "UP",
            "0" : "DOWN",
        }
    },
    "mdio_status" : {
        "decode" : {
            "1" : "NOT OK",
            "0" : "OK",
        }
    },
}

PREFIX = "/sys/logic_dev/"

class PhyCollector(CollectorBase):
    name = 'phy'

    def __init__(self, phy_list = None):
        self.phy_list = phy_list

    def collect(self):
        logger.info("Starting phy collect...")
        info = {}

        if not self.phy_list:
            self.phy_list = []
            self.phy_dirs = glob.glob(os.path.join(PREFIX, '*_phy'))
            for phy_dir in self.phy_dirs:
                self.phy_list.append(phy_dir.split("/")[-1])

        logger.info("phy_list: %s", self.phy_list)
        for phy_name in self.phy_list:
            prefix = "%s%s/" % (PREFIX, phy_name)
            for base, dirs, files in os.walk(prefix):
                files.sort()

                for sysfs_name in files:
                    item_config = PHY_COLL_INFO.get(sysfs_name, {})
                    display_name = "%s/%s" % (phy_name, sysfs_name)
                    status, val = run_cmd('cat %s%s' % (prefix, sysfs_name))
                    if status is False:
                        info[display_name] = val
                    else:
                        decode = item_config.get("decode")
                        if decode:
                            info[display_name] = decode.get(val, ("[%s] decode fail" % val))
                        else:
                            info[display_name] = val
        return info