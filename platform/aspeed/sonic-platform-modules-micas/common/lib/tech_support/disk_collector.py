#!/usr/bin/env python_nos
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd


class DiskCollector(CollectorBase):
    name = 'disk'

    def collect(self):
        logger.info("Starting disk collect...")
        info = {}
        status, lsblk = run_cmd('lsblk -o NAME,SIZE,TYPE,MOUNTPOINT -a')
        info['lsblk'] = lsblk

        status, blkid = run_cmd('blkid')
        info['blkid'] = blkid

        status, df = run_cmd('df -h')
        info['df'] = df


        status, smartctl_version = run_cmd('smartctl --version')
        info['smartctl_version'] = smartctl_version

        # run smartctl -a for each disk if available
        status, ls = run_cmd("ls /dev/sd* 2>/dev/null")
        if status is False:
            info['ls'] = ls
        else:
            for d in ls.split():
                status, tmp_val = run_cmd(f'smartctl -a {d}')
                info[d] = tmp_val
        logger.info("Finish disk collect...")
        return info
