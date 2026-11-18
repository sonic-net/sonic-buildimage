#!/usr/bin/env python3

import subprocess
import sys
import time

from swsscommon.swsscommon import ConfigDBConnector

PTP4L_CONFIGURATION = 'configuration'
PTP4L_PTP = 'ptp'
PTP4L_DEPENDENTS = ('telemetry', 'ptp_counters')
PTP4L_RESTART_DELAY_SEC = 5

db = ConfigDBConnector()
db.connect()


def _supervisorctl(*args):
    return subprocess.run(
        ['supervisorctl', *args],
        capture_output=True,
        text=True,
    )


def _supervisorctl_restart(programs):
    program_list = list(programs)
    result = _supervisorctl('restart', *program_list)
    if result.returncode != 0:
        print(
            'ptp_agent: supervisorctl restart {} failed ({}): {}'.format(
                ' '.join(program_list),
                result.returncode,
                (result.stderr or result.stdout).strip(),
            ),
            file=sys.stderr,
        )
        return False
    output = (result.stdout or '').strip()
    if output:
        print('ptp_agent: supervisorctl restart: {}'.format(output))
    return True


def restart_ptp4l_group():
    if not _supervisorctl_restart((PTP4L_CONFIGURATION,)):
        return False
    time.sleep(PTP4L_RESTART_DELAY_SEC)

    if not _supervisorctl_restart((PTP4L_PTP,)):
        return False
    time.sleep(PTP4L_RESTART_DELAY_SEC)

    if not _supervisorctl_restart(PTP4L_DEPENDENTS):
        return False

    print(
        'ptp_agent: successfully restarted ptp4l services '
        '({} -> {} -> {})'.format(
            PTP4L_CONFIGURATION,
            PTP4L_PTP,
            ', '.join(PTP4L_DEPENDENTS),
        ),
    )
    return True


def apply_config():
    restart_ptp4l_group()


db.subscribe('PTP', lambda table, key, data: apply_config())
db.subscribe('PTP_PORT', lambda table, key, data: apply_config())
db.subscribe('PTP_UNICAST_MASTER_TABLE_IP', lambda table, key, data: apply_config())
db.listen()
