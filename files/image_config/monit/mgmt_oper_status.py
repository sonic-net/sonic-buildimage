#!/usr/bin/env python3

"""
"""
import os
import sys
import syslog

import sonic_platform.platform
from swsscommon.swsscommon import SonicV2Connector


def _get_kernel_operstate(intf):
    """!
    Read the kernel-reported operstate for a network interface.

    @return 'up', 'down', or other kernel operstate string.
    """
    path = "/sys/class/net/{}/operstate".format(intf)
    if not os.path.exists(path):
        syslog.syslog(syslog.LOG_ERR,
                      "operstate path does not exist: {}".format(path))
        return "unknown"
    with open(path, "r") as fh:
        return fh.readline().strip().lower()


def _has_platform_mgmt_link_check():
    """!
    Check whether this platform overrides the kernel-reported management port
    link status via the platform API (get_management_port_link_status_override).
    """
    try:
        chassis = sonic_platform.platform.Platform().get_chassis()
    except Exception:
        return False
    return hasattr(chassis, "get_management_port_link_status_override")


def _platform_mgmt_link_check(intf):
    """!
    Query the platform for the real management port link status via the
    platform API. If the platform provides an override (returns True/False),
    use it instead of the kernel operstate. If not implemented (returns None)
    or on error, return None to fall back to the kernel operstate.

    @return 'up' if the platform reports link up;
            'down' if the platform reports link down;
            None if the platform does not override or on error.
    """
    try:
        chassis = sonic_platform.platform.Platform().get_chassis()
        is_up = chassis.get_management_port_link_status_override(intf)
    except Exception as e:
        syslog.syslog(syslog.LOG_WARNING,
                      "get_management_port_link_status_override failed: %s" % str(e))
        return None
    if is_up is None:
        return None
    syslog.syslog(syslog.LOG_DEBUG,
                  "Platform mgmt link check: %s" % ("up" if is_up else "down"))
    return "up" if is_up else "down"


def main():
    db = SonicV2Connector(use_unix_socket_path=True)
    db.connect('CONFIG_DB')
    db.connect('STATE_DB')
    mgmt_ports_keys = db.keys(db.CONFIG_DB, 'MGMT_PORT|*')
    if not mgmt_ports_keys:
        syslog.syslog(syslog.LOG_DEBUG, 'No management interface found')
    else:
        try:
            mgmt_ports = [key.split('MGMT_PORT|')[-1] for key
                          in mgmt_ports_keys]
            for port in mgmt_ports:
                state_db_mgmt_keys = db.keys(db.STATE_DB, 'MGMT_PORT_TABLE|*')
                state_db_key = "MGMT_PORT_TABLE|{}".format(port)
                config_db_key = "MGMT_PORT|{}".format(port)
                config_db_mgmt = db.get_all(db.CONFIG_DB, config_db_key)
                state_db_mgmt = db.get_all(db.STATE_DB, state_db_key) if state_db_key in state_db_mgmt_keys else {}

                # Sync fields from CONFIG_DB MGMT_PORT table to STATE_DB MGMT_PORT_TABLE
                for field in config_db_mgmt:
                        if field != 'oper_status':
                            # Update STATE_DB if port is not present or value differs from
                            # CONFIG_DB
                            if (field in state_db_mgmt and state_db_mgmt[field] != config_db_mgmt[field]) \
                            or field not in state_db_mgmt:
                                db.set(db.STATE_DB, state_db_key, field, config_db_mgmt[field])

                # Update oper status if modified
                prev_oper_status = state_db_mgmt.get('oper_status', 'unknown')
                override = _platform_mgmt_link_check(port) if _has_platform_mgmt_link_check() else None
                current_oper_status = override if override is not None else _get_kernel_operstate(port)
                if current_oper_status != prev_oper_status:
                    db.set(db.STATE_DB, state_db_key, 'oper_status', current_oper_status)
                    log_level = syslog.LOG_INFO if current_oper_status == 'up' else syslog.LOG_WARNING
                    syslog.syslog(log_level, "mgmt_oper_status: {}".format(current_oper_status))

        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "mgmt_oper_status exception : {}".format(str(e)))
            db.set(db.STATE_DB, state_db_key, 'oper_status', 'unknown')
            sys.exit(1)


if __name__ == "__main__":
    main()
    sys.exit(0)
