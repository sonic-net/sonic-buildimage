#!/usr/bin/env python3

"""
     port_notify
     port notify status change for SONiC
"""

try:
    from datetime import datetime
    from swsscommon import swsscommon
    from sonic_py_common import daemon_base, logger
except ImportError as e:
    raise ImportError (str(e) + " - required module not found")

#
# Constants ====================================================================
#

SYSLOG_IDENTIFIER = "port_notify"


STATE_PORT_TABLE = 'PORT_TABLE'


RJ45_PORT_START = 0;
RJ45_PORT_END = 47;

# Global logger class instance
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

XCVR_STATE_EMPTY   = 0
XCVR_STATE_ERROR   = 1
XCVR_STATE_INCOMP  = 2
XCVR_STATE_CONFIG  = 3
XCVR_STATE_READY   = 4
XCVR_STATE_TIMEOUT = 5

xcvr_state_tbl = {
    XCVR_STATE_EMPTY:   { "xcvr_state": "N/A",          "xcvr_app_status": "down" },
    XCVR_STATE_ERROR:   { "xcvr_state": "Error",        "xcvr_app_status": "down" },
    XCVR_STATE_INCOMP:  { "xcvr_state": "Incompatible", "xcvr_app_status": "up" },
    XCVR_STATE_CONFIG:  { "xcvr_state": "Config",       "xcvr_app_status": "down" },
    XCVR_STATE_TIMEOUT: { "xcvr_state": "Timeout",      "xcvr_app_status": "up" },
    XCVR_STATE_READY:   { "xcvr_state": "Ready",        "xcvr_app_status": "up" }
}

# Wait for port init is done
def wait_for_port_init_done():
    # Connect to APPL_DB and subscribe to PORT table notifications
    appl_db = daemon_base.db_connect("APPL_DB")

    sel = swsscommon.Select()
    sst = swsscommon.SubscriberStateTable(appl_db, swsscommon.APP_PORT_TABLE_NAME)
    sel.addSelectable(sst)

    # Make sure this daemon started after all port configured
    while True:
        (state, c) = sel.select(1000)
        if state == swsscommon.Select.TIMEOUT:
            continue
        if state != swsscommon.Select.OBJECT:
            helper_logger.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
            continue

        (key, op, fvp) = sst.pop()

        # Wait until PortInitDone
        if key in ["PortInitDone"]:
            break

def notify_port_xcvr_status(port_name, app_status_port_tbl, state_port_tbl, flag):

    fvs = swsscommon.FieldValuePairs([("xcvr_status", xcvr_state_tbl[flag]["xcvr_app_status"])])
    tm = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    state_fvs = swsscommon.FieldValuePairs([("xcvr_status", xcvr_state_tbl[flag]["xcvr_state"]), ("xcvr_time", tm)])

    state_port_tbl.set(port_name, state_fvs)

    app_status_port_tbl.set(port_name, fvs)

    helper_logger.log_notice("Port {} xcvr_app_status change to {}".format(port_name, xcvr_state_tbl[flag]["xcvr_app_status"]))
    return True


def main():
    helper_logger.log_notice("Start port_notify")
    # Connect to APP_DB and create transceiver dom info table
    appl_db = daemon_base.db_connect("APPL_DB")

    app_status_port_tbl = swsscommon.ProducerStateTable(appl_db,
                                                     swsscommon.APP_PORT_APP_STATUS_TABLE_NAME)

    state_db = daemon_base.db_connect("STATE_DB")
    state_port_tbl = swsscommon.Table(state_db, STATE_PORT_TABLE)

    # Wait for PortInitDone
    wait_for_port_init_done()

    for port in range(RJ45_PORT_START, RJ45_PORT_END+1):
        #print("Ethernet{}".format(port))
        notify_port_xcvr_status("Ethernet{}".format(port), app_status_port_tbl, state_port_tbl, XCVR_STATE_READY)

    helper_logger.log_notice("End port_notify")


if __name__ == '__main__':
    main()
