#!/usr/bin/python3

import argparse
import ipaddress
import psutil
import os
import signal
import syslog
import time

from swsscommon.swsscommon import SubscriberStateTable, TableConsumable, Select
from swsscommon.swsscommon import DBConnector, Table, STATE_FDB_TABLE_NAME, APP_PORT_TABLE_NAME, APP_VLAN_MEMBER_TABLE_NAME, APP_INTF_TABLE_NAME

from swsscommon import swsscommon

class DnsmasqStaticHostMonitor(object):

    def __init__(self, hosts_update_delay):
        self.appl_db = DBConnector("APPL_DB", 0)
        self.state_db = DBConnector("STATE_DB", 0)

        self.selector = Select()
        self.callbacks = dict()
        self.subscriber_map = dict()

        self.next_hosts_update = None
        self.hosts_update_delay = hosts_update_delay
        self.changes_since_last_update = False

        # Build a lookup table from port to port index. Also determine the
        # "base" port index.
        port_table = Table(self.appl_db, APP_PORT_TABLE_NAME)
        port_keys = port_table.getKeys()
        self.port_index_lookup = {}
        self.min_port_index = None
        for port in port_keys:
            # Get the index that corresponds to this ethernet port
            (status, port_index) = port_table.hget(port, 'index')
            if not status:
                syslog.syslog(syslog.LOG_WARNING, f"Unable to get port index for {port}")
                continue
            port_index = int(port_index)
            self.port_index_lookup[port] = port_index
            if self.min_port_index is None or port_index < self.min_port_index:
                self.min_port_index = port_index

    def new_host_update(self, key, op, data):
        self.changes_since_last_update = True
        current_time = time.clock_gettime(time.CLOCK_MONOTONIC)
        if self.next_hosts_update is None or current_time > self.next_hosts_update:
            self.update_static_dnsmasq_hosts_file()
        else:
            syslog.syslog(syslog.LOG_INFO | syslog.LOG_USER,
                    f"Not updating dnsmasq.hosts since the last update was within the last {self.hosts_update_delay} seconds")

    def update_static_dnsmasq_hosts_file(self):
        syslog.syslog(syslog.LOG_INFO | syslog.LOG_USER,
                "Updating dnsmasq.hosts with current MAC addresses")

        fdb_table = Table(self.state_db, STATE_FDB_TABLE_NAME)

        with open("/etc/dnsmasq.hosts", "w") as f:
            # Get all known MAC addresses
            vlan_mac_addresses = fdb_table.getKeys()
            ip_mac_dict = {}
            for vlan_mac_address in vlan_mac_addresses:
                (vlan, mac_address) = vlan_mac_address.split(":", maxsplit=1)
                (status, port) = fdb_table.hget(vlan_mac_address, 'port')
                if not status:
                    continue

                vlan_ip_addresses = self.appl_db.keys(f"{APP_INTF_TABLE_NAME}:{vlan}:*")
                ipv4_network = None
                for vlan_ip_address in vlan_ip_addresses:
                    ipv4_network = ipaddress.ip_interface(vlan_ip_address.split(":", maxsplit=2)[2])
                    if ipv4_network.version != 4:
                        continue
                    break

                if not ipv4_network:
                    syslog.syslog(syslog.LOG_INFO, f"No IPv4 addresses found for {vlan}, skipping")
                    continue

                ipv4_supernet = ipv4_network.network
                if ipv4_supernet.prefixlen > 24:
                    ipv4_supernet = ipv4_supernet.supernet(new_prefix=24)

                if port not in self.port_index_lookup:
                    syslog.syslog(syslog.LOG_ERR, f"Got port {port} that doesn't map to a port index, skipping")
                    continue

                # Write a host entry
                ipv4_device_address = ipv4_supernet[(self.port_index_lookup[port] - self.min_port_index) * 4 + 1]
                if ipv4_device_address not in ipv4_network.network.hosts():
                    syslog.syslog(syslog.LOG_ERR, f"Calculated address {ipv4_device_address} not part of {ipv4_network} for {mac_address}")
                    continue

                # If different mac get same ip, always respect mac first met
                if ipv4_device_address in ip_mac_dict:
                    syslog.syslog(syslog.LOG_INFO, f"Duplicate ip address {ipv4_device_address} which has been assigned"
                                  f" to {ip_mac_dict[ipv4_device_address]}, skip {mac_address}")
                    continue

                ip_mac_dict[ipv4_device_address] = mac_address

            for ip, mac in ip_mac_dict.items():
                f.write(f"{mac},{ip},15m\n")


        # Update the next time we'll update /etc/dnsmasq.hosts
        current_time = time.clock_gettime(time.CLOCK_MONOTONIC)
        self.next_hosts_update = current_time + self.hosts_update_delay

        for proc in psutil.process_iter(['pid', 'name']):
           if proc.info["name"] == "dnsmasq":
               os.kill(proc.info["pid"], signal.SIGHUP)
               break

        self.changes_since_last_update = False

    def subscribe(self, db, table, callback, pri):
        try:
            if table not in self.callbacks:
                self.callbacks[table] = []
                subscriber = SubscriberStateTable(db, table, TableConsumable.DEFAULT_POP_BATCH_SIZE, pri)
                self.selector.addSelectable(subscriber) # Add to the Selector
                self.subscriber_map[subscriber.getFd()] = (subscriber, table) # Maintain a mapping b/w subscriber & fd

            self.callbacks[table].append(callback)
        except Exception as err:
            syslog.syslog(syslog.LOG_ERR, "Subscribe to table {} failed with error {}".format(table, err))

    def start(self):
        while True:
            # Keep a low timeout, so that it can respond to Ctrl-C in a reasonable
            # amount of time
            select_timeout_msec = 5 * 1000
            hosts_update_delay_msec = self.hosts_update_delay * 1000
            if hosts_update_delay_msec < select_timeout_msec:
                select_timeout_msec = self.hosts_update_delay_msec
            state, selectable_ = self.selector.select(select_timeout_msec)
            if state == self.selector.TIMEOUT:
                if self.changes_since_last_update:
                    # Add a bit of a hack here where if we need to update the hosts file
                    # because of changes since the last update to it, then manually do
                    # the update here.
                    current_time = time.clock_gettime(time.CLOCK_MONOTONIC)
                    if self.next_hosts_update is None or current_time >= self.next_hosts_update:
                        self.update_static_dnsmasq_hosts_file()
                continue
            elif state == self.selector.ERROR:
                syslog.syslog(syslog.LOG_ERR,
                        "error returned by select")
                continue

            fd = selectable_.getFd()
            # Get the Corresponding subscriber & table
            subscriber, table = self.subscriber_map.get(fd, (None, ""))
            if not subscriber:
                syslog.syslog(syslog.LOG_ERR,
                        "No Subscriber object found for fd: {}, subscriber map: {}".format(fd, subscriber_map))
                continue
            key, op, fvs = subscriber.pop()
            # Get the registered callback
            cbs = self.callbacks.get(table, None)
            for callback in cbs:
                callback(table, key, op, dict(fvs))

    def register_callbacks(self):
        #self.subscribe('KDUMP', lambda table, key, op, data: self.kdump_handler(key, op, data), HOSTCFGD_MAX_PRI)
        self.subscribe(self.state_db, "FDB_TABLE", lambda table, key, op, data: self.new_host_update(key, op, data), 1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start a script to update /etc/dnsmasq.hosts with the currently-known MAC addresses.')
    parser.add_argument('--update-every', type=int, default=60, help='Set the frequency (in seconds) to update /etc/dnsmasq.hosts. (default: 60 seconds, or 1 minute)')

    args = parser.parse_args()

    monitor = DnsmasqStaticHostMonitor(args.update_every)
    monitor.register_callbacks()
    monitor.update_static_dnsmasq_hosts_file()
    monitor.start()
