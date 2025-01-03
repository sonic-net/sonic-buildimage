#!/usr/bin/env python3

"""
"""
import sys
import subprocess
import syslog

from sonic_py_common import multi_asic, device_info
from swsscommon.swsscommon import SonicV2Connector


def main():
    db = SonicV2Connector(use_unix_socket_path=True)
    db.connect('CONFIG_DB')
    db.connect('STATE_DB')
    mgmt_ports_keys = db.keys(db.CONFIG_DB, 'MGMT_PORT|*' )
    if not mgmt_ports_keys:
        syslog.syslog(syslog.LOG_DEBUG, 'No management interface found')
    else:
        try:
            mgmt_ports = [key.split('MGMT_PORT|')[-1] for key in mgmt_ports_keys]
            for port in mgmt_ports:
                state_db_mgmt_port = db.keys(db.STATE_DB, 'MGMT_PORT_TABLE|*' )
                state_db_key = "MGMT_PORT_TABLE|{}".format(port)
                prev_oper_status = 'unknown'
                if state_db_key in state_db_mgmt_port:
                    prev_oper_status = db.get(db.STATE_DB, state_db_key, 'oper_status')
                port_operstate_path = '/sys/class/net/{}/operstate'.format(port)
                oper_status = subprocess.run(['cat', port_operstate_path], capture_output=True, text=True)
                current_oper_status = oper_status.stdout.strip()
                if current_oper_status != prev_oper_status:
                    db.set(db.STATE_DB, state_db_key, 'oper_status', current_oper_status)
                    log_level = syslog.LOG_INFO if current_oper_status == 'up' else syslog.LOG_WARNING 
                    syslog.syslog(log_level, "mgmt_oper_status: {}".format(current_oper_status))
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "mgmt_oper_status exception : {}".format(str(e)))
            db.set(db.STATE_DB, state_db_key, 'oper_status', 'unknown')


if __name__ == "__main__":
    main()
    sys.exit(0)
