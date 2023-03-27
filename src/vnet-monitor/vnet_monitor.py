#!/usr/bin/env python3

from scapy.all import *
import configutil
from vnet_monitor_base import *
from swsscommon import swsscommon
import signal

APP_DB_VNET_MONITOR_TABLE_NAME = "VNET_MONITOR_TABLE"
STATE_DB_VNET_MONITOR_TABLE_NAME = "VNET_MONITOR_TABLE"

def state_db_table(table_name):
    state_db = swsscommon.DBConnector("STATE_DB", 0, False)
    vnet_monitor_table = swsscommon.Table(state_db, table_name)
    return vnet_monitor_table

g_task_runner = None
g_db_monitor = None
g_reply_monitor = None


def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    logger_helper.log_notice("Vnet monitor going to quit")
    # Ignore any exception during quit
    try:
        if g_task_runner:
            g_task_runner.stop()
        if g_reply_monitor:
            g_reply_monitor.stop()
        if g_db_monitor:
            g_db_monitor.stop()
    except:
        pass


def main():
    """
    The entry of vnet_monitor
    """
    # Install signal handler
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    global g_ping_stats, g_db_monitor, g_task_runner, g_reply_monitor
    state_db_tbl = state_db_table(STATE_DB_VNET_MONITOR_TABLE_NAME)
    g_ping_stats.set_state_db_table(state_db_tbl)
    g_task_runner = TaskRunner(g_ping_stats)
    t1_mac = configutil.get_localhost_mac()
    t1_loopback_v4 = configutil.get_loopback_ip(af=4)
    t1_loopback_v6 = configutil.get_loopback_ip(af=6)
    # Initialize the ping task
    TaskPing.init(t1_loopback_v4)
    g_db_monitor = DBMonitor(table_name=APP_DB_VNET_MONITOR_TABLE_NAME,
                            t1_mac=t1_mac,
                            t1_loopback={4: t1_loopback_v4, 6: t1_loopback_v6},
                            vni=DEFAULT_VNI,
                            task_runner=g_task_runner,
                            cached_stats=g_ping_stats)
    
    g_reply_monitor = ReplyMonitor({4:t1_loopback_v4, 6:t1_loopback_v6}, DEFAULT_UDP_PORT)
    
    # Start task runner
    g_task_runner.start()
    # Start watching reply
    g_reply_monitor.start()
    logger_helper.log_notice("Vnet monitor started")
    # Start watching DB (Blocked)
    g_db_monitor.start()
    # Destroy the shared fd in ping task
    TaskPing.teardown()
    logger_helper.log_notice("Vnet monitor quited")


if __name__ == "__main__":
    main()
