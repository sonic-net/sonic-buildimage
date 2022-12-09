#!/usr/bin/env python3

"""
    bootstrap-asic
"""
try:
    import re
    import sys
    from sonic_py_common import daemon_base
    from swsscommon import swsscommon
    from sonic_py_common import multi_asic
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

#
# Constants ====================================================================
#
SYSLOG_IDENTIFIER = 'asic_status.py'
CHASSIS_FABRIC_ASIC_INFO_TABLE = 'CHASSIS_FABRIC_ASIC_TABLE'
SELECT_TIMEOUT_MSECS = 5000

def main():
    logger = Logger(SYSLOG_IDENTIFIER)
    logger.set_min_log_priority_info()

    if len(sys.argv) != 3:
        raise Exception('Pass service and valid asic-id as arguments')

    service = sys.argv[1]
    args_asic_id = sys.argv[2]

    # Get num asics
    num_asics = multi_asic.get_num_asics()
    if num_asics == 0:
        logger.log_error('Detected no asics on this platform for service {}'.format(service))
        sys.exit(1)

    # Connect to STATE_DB and subscribe to chassis-module table notifications
    state_db = daemon_base.db_connect("CHASSIS_STATE_DB")

    sel = swsscommon.Select()
    sst = swsscommon.SubscriberStateTable(state_db, CHASSIS_FABRIC_ASIC_INFO_TABLE)
    sel.addSelectable(sst)

    while True:
        (state, c) = sel.select(SELECT_TIMEOUT_MSECS)
        if state == swsscommon.Select.TIMEOUT:
            continue
        if state != swsscommon.Select.OBJECT:
            continue

        (asic_key, asic_op, asic_fvp) = sst.pop()
        asic_id=re.search(r'\d+$', asic_key)
        global_asic_id = asic_id.group(0)

        if asic_op == 'SET':
            asic_fvs = dict(asic_fvp)
            asic_name = asic_fvs.get('name')
            if asic_name is None:
                logger.log_info('Unable to get asic_name for asic{}'.format(global_asic_id))
                continue

            if asic_name.startswith('FABRIC-CARD') is False:
                logger.log_info('Skipping module with asic_name {} for asic{}'.format(asic_name, global_asic_id))
                continue

            if (global_asic_id == args_asic_id):
                logger.log_info('Detected asic{} is online'.format(global_asic_id))
                sys.exit(0)
        elif asic_op == 'DEL':
            logger.log_info('Detected asic{} is offline'.format(global_asic_id))
            sys.exit(1)
        else:
            continue

if __name__ == "__main__":
    main()
