#!/usr/bin/env python3

from scapy.all import *
import configutil
from vnet_monitor_base import *
from swsscommon import swsscommon
import signal

APP_DB_VNET_MONITOR_TABLE_NAME = "VNET_MONITOR_TABLE"
STATE_DB_VNET_MONITOR_TABLE_NAME = "VNET_MONITOR_TABLE"
CONFIG_DB_BGP_DEVICE_GLOBAL_NAME = "BGP_DEVICE_GLOBAL"

def state_db_table(table_name):
    state_db = swsscommon.DBConnector("STATE_DB", 0, False)
    vnet_monitor_table = swsscommon.Table(state_db, table_name)
    return vnet_monitor_table

g_db_monitor = None
g_reply_monitor = None
g_ping_task = None


def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    logger_helper.log_notice("Vnet monitor going to quit")
    # Ignore any exception during quit
    try:
        if g_ping_task:
            g_ping_task.teardown()
        if g_reply_monitor:
            g_reply_monitor.stop()
        if g_db_monitor:
            g_db_monitor.stop()
        if g_ping_stats:
            g_ping_stats.teardown()
    except:
        pass


def main():
    """
    The entry of vnet_monitor
    """
    # Install signal handler
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    global g_ping_stats, g_db_monitor, g_reply_monitor, g_ping_task_list, g_ping_task
    state_db_tbl = state_db_table(STATE_DB_VNET_MONITOR_TABLE_NAME)
    g_ping_stats.set_state_db_table(state_db_tbl)
    t1_mac = configutil.get_localhost_mac()
    t1_loopback_v4 = configutil.get_loopback_ip(af=4)
    t1_loopback_v6 = configutil.get_loopback_ip(af=6)
    # Initialize the ping task
    g_ping_task = TaskPing(task_queue=g_ping_task_list,
                         t1_mac=t1_mac,
                         t1_loopback_v4=t1_loopback_v4,
                         t1_loopback_v6=t1_loopback_v6)

    g_db_monitor = DBMonitor(table_name=APP_DB_VNET_MONITOR_TABLE_NAME,
                            tsa_table_name=CONFIG_DB_BGP_DEVICE_GLOBAL_NAME,
                            t1_mac=t1_mac,
                            t1_loopback={4: t1_loopback_v4, 6: t1_loopback_v6},
                            vni=DEFAULT_VNI,
                            cached_stats=g_ping_stats,
                            task_list=g_ping_task_list)
    
    g_reply_monitor = ReplyMonitor({4:t1_loopback_v4, 6:t1_loopback_v6}, DEFAULT_UDP_PORT)
    
    # Start watching reply
    g_ping_task.init()
    g_reply_monitor.start()
    logger_helper.log_notice("Vnet monitor started")
    # Start watching DB (Blocked)
    g_db_monitor.start()

    logger_helper.log_notice("Vnet monitor quited")


if __name__ == "__main__":
    main()
