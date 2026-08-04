#!/usr/bin/env python3

""""
Description: restore_nat_entries.py -- restoring nat entries table into kernel during system warm reboot.
    The script is started by supervisord in nat docker when the docker is started.
    It does not do anything in case neither system nor nat warm restart is enabled.
    In case nat warm restart enabled only, it sets the stateDB flag so natsyncd can continue
    the reconciation process.
    In case system warm reboot is enabled, it will try to restore the nat entries table into kernel
    , then it sets the stateDB flag for natsyncd to continue the
    reconciliation process.
"""

import os
import re
import subprocess
import sys

from sonic_py_common.logger import Logger
from swsscommon import swsscommon

SYSLOG_IDENTIFIER = os.path.basename(__file__)

WARM_BOOT_FILE_DIR = '/var/warmboot/nat/'
NAT_WARM_BOOT_FILE = 'nat_entries.dump'
IP_PROTO_TCP = '6'

MATCH_CONNTRACK_ENTRY = '^(\w+)\s+(\d+).*src=([\d.]+)\s+dst=([\d.]+)\s+sport=(\d+)\s+dport=(\d+).*src=([\d.]+)\s+dst=([\d.]+)\s+sport=(\d+)\s+dport=(\d+)'

# Global logger instance
logger = Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_info()


def add_nat_conntrack_entry_in_kernel(ipproto, srcip, dstip, srcport, dstport, natsrcip, natdstip, natsrcport, natdstport):
    # pyroute2 doesn't have support for adding conntrack entries via netlink yet. So, invoking the conntrack utility to add the entries.
    state = []
    if (ipproto == IP_PROTO_TCP):
        state = ['--state', 'ESTABLISHED']
    ctcmd = ['conntrack', '-I', '-n', natdstip + ':' + natdstport, '-g', natsrcip + ':' + natsrcport, \
        '--protonum', ipproto] + state + ['--timeout', '432000', '--src', srcip, '--sport', srcport, \
        '--dst', dstip, '--dport', dstport, '-u', 'ASSURED']
    subprocess.call(ctcmd)
    logger.log_info("Restored NAT entry: {}".format(ctcmd))


# Set the statedb "NAT_RESTORE_TABLE|Flags", so natsyncd can start reconciliation
def set_statedb_nat_restore_done():
    statedb = swsscommon.DBConnector("STATE_DB", 0)
    tbl = swsscommon.Table(statedb, "NAT_RESTORE_TABLE")
    fvs = swsscommon.FieldValuePairs([("restored", "true")])
    tbl.set("Flags", fvs)
    return


# This function is to restore the kernel nat entries based on the saved nat entries.
def restore_update_kernel_nat_entries(filename):
    # Read the entries from nat_entries.dump file and add them to kernel
    conntrack_match_pattern = re.compile(r'{}'.format(MATCH_CONNTRACK_ENTRY))
    with open(filename, 'r') as fp:
        for line in fp:
            ctline = conntrack_match_pattern.findall(line)
            if not ctline:
                continue
            cmdargs = list(ctline.pop(0))
            proto = cmdargs.pop(0)
            if proto not in ('tcp', 'udp'):
                continue
            add_nat_conntrack_entry_in_kernel(*cmdargs)


def main():
    logger.log_info("restore_nat_entries service is started")

    # Use warmstart python binding to check warmstart information
    warmstart = swsscommon.WarmStart()
    warmstart.initialize("natsyncd", "nat")
    warmstart.checkWarmStart("natsyncd", "nat", False)

    # if swss or system warm reboot not enabled, don't run
    if not warmstart.isWarmStart():
        logger.log_info("restore_nat_entries service is skipped as warm restart not enabled")
        return

    # NAT restart not system warm reboot, set statedb directly
    if not warmstart.isSystemWarmRebootEnabled():
        set_statedb_nat_restore_done()
        logger.log_info("restore_nat_entries service is done as system warm reboot not enabled")
        return

    # Program the nat conntrack entries in the kernel by reading the
    # entries from nat_entries.dump
    try:
        restore_update_kernel_nat_entries(WARM_BOOT_FILE_DIR + NAT_WARM_BOOT_FILE)
    except Exception as e:
        logger.log_error(str(e))
        sys.exit(1)

    # Remove the dump file after restoration
    os.remove(WARM_BOOT_FILE_DIR + NAT_WARM_BOOT_FILE)

    # set statedb to signal other processes like natsyncd
    set_statedb_nat_restore_done()
    logger.log_info("restore_nat_entries service is done for system warmreboot")
    return


if __name__ == '__main__':
    main()
