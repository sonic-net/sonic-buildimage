#! /usr/bin/env python3
"""
Adds neighbor to kernel for undeliverable tunnel packets

When receiving tunnel packets, if the hardware doesn't contain neighbor
information for the inner packet's destination IP, the entire encapsulated
packet is trapped to the CPU. In this case, we should ping the inner
destination IP to trigger the process of obtaining neighbor information
"""
import subprocess
import sys
import time
from datetime import datetime
from ipaddress import ip_interface
from queue import Queue
from threading import Lock, Event, Thread

from swsscommon.swsscommon import ConfigDBConnector, SonicV2Connector, \
                                  DBConnector, Select, SubscriberStateTable
from sonic_py_common import logger as log

from pyroute2 import IPRoute
from pyroute2.netlink.exceptions import NetlinkError
from scapy.layers.inet import IP
from scapy.layers.inet6 import IPv6
from scapy.sendrecv import AsyncSniffer


logger = log.Logger()

STATE_DB = 'STATE_DB'
APPL_DB = 'APPL_DB'
COUNTERS_DB = 'COUNTERS_DB'
TUNNEL_PKT_COUNTER_TEMPLATE = 'COUNTERS{}IPINIP_TUNNEL_CPU_PKTS'
COUNTER_KEY = 'RX_COUNT'
PORTCHANNEL_INTERFACE_TABLE = 'PORTCHANNEL_INTERFACE'
TUNNEL_TABLE = 'TUNNEL'
PEER_SWITCH_TABLE = 'PEER_SWITCH'
INTF_TABLE_TEMPLATE = 'INTERFACE_TABLE|{}|{}'
LAG_TABLE = 'LAG_TABLE'
STATE_KEY = 'state'
TUNNEL_TYPE_KEY = 'tunnel_type'
DST_IP_KEY = 'dst_ip'
ADDRESS_IPV4_KEY = 'address_ipv4'
OPER_STATUS_KEY = 'oper_status'
IPINIP_TUNNEL = 'ipinip'
RTM_NEWLINK = 'RTM_NEWLINK'
SELECT_TIMEOUT = 1000

nl_msgs = Queue()
portchannel_intfs = None

def add_msg_to_queue(target, msg):
    """
    Adds a netlink message to a queue

    Args:
        target: unused, needed by NDB API
        msg: a netlink message
    """

    if msg.get_attr('IFLA_IFNAME') in portchannel_intfs:
        nl_msgs.put(msg)

class TunnelPacketHandler(object):
    """
    This class handles unroutable tunnel packets that are trapped
    to the CPU from the ASIC.
    """

    def __init__(self):
        self.config_db = ConfigDBConnector()
        self.config_db.connect()
        self.state_db = SonicV2Connector()
        self.state_db.connect(STATE_DB)
        self.counters_db = SonicV2Connector()
        self.counters_db.connect(COUNTERS_DB)
        counters_db_separator = self.counters_db.get_db_separator(COUNTERS_DB)
        self.tunnel_counter_table = TUNNEL_PKT_COUNTER_TEMPLATE.format(counters_db_separator)
        self._portchannel_intfs = None
        self.up_portchannels = None
        self.netlink_api = IPRoute()
        self.sniffer = None
        self.self_ip = ''
        self.packet_filter = ''
        self.sniff_intfs = set()
        self.pending_cmds = Queue()

        global portchannel_intfs
        portchannel_intfs = [name for name, _ in self.portchannel_intfs]

    @property
    def portchannel_intfs(self):
        """
        Gets all portchannel interfaces and IPv4 addresses in config DB

        Returns:
            (list) Tuples of a portchannel interface name (str) and
                   associated IPv4 address (str)
        """
        if self._portchannel_intfs is None:
            intf_keys = self.config_db.get_keys(PORTCHANNEL_INTERFACE_TABLE)
            portchannel_intfs = []

            for key in intf_keys:
                if isinstance(key, tuple) and len(key) > 1:
                    if ip_interface(key[1]).version == 4:
                        portchannel_intfs.append(key)

            self._portchannel_intfs = portchannel_intfs

        return self._portchannel_intfs

    def get_intf_name(self, msg):
        """
        Gets the interface name for a netlink msg

        Returns:
            (str) The interface name, or the empty string if no interface
                  name was found
        """
        attr_list = msg.get('attrs', list())

        for attribute, val in attr_list:
            if attribute == 'IFLA_IFNAME':
                return val

        return ''

    def get_up_portchannels(self):
        """
        Returns the portchannels which are operationally up

        Returns:
            (list) of interface names which are up, as strings
        """
        portchannel_intf_names = [name for name, _ in self.portchannel_intfs]
        link_statuses = []
        for intf in portchannel_intf_names:
            try:
                status = self.netlink_api.link("get", ifname=intf)
            except NetlinkError:
                # Continue if we find a non-existent interface since we don't
                # need to listen on it while it's down/not created. Once it comes up,
                # we will get another netlink message which will trigger this check again
                logger.log_notice("Skipping non-existent interface {}".format(intf))
                continue
            link_statuses.append(status[0])
        up_portchannels = set()

        for status in link_statuses:
            if status.get_attr('IFLA_OPERSTATE').lower() == 'up':
                up_portchannels.add(status.get_attr('IFLA_IFNAME'))

        return up_portchannels

    def all_portchannels_established(self):
        """
        Checks if the portchannel interfaces are established

        Note that this status does not indicate operational state
        Returns:
            (bool) True, if all interfaces are established
                   False, otherwise
        """
        intfs = self.portchannel_intfs
        for intf in intfs:
            intf_table_name = INTF_TABLE_TEMPLATE.format(intf[0], intf[1])
            intf_state = self.state_db.get(
                                STATE_DB,
                                intf_table_name,
                                STATE_KEY
                        )

            if intf_state and intf_state.lower() != 'ok':
                return False

        return True

    def wait_for_portchannels(self, interval=5, timeout=60):
        """
        Continuosly checks if all portchannel host interfaces are established

        Args:
            interval: the interval (in seconds) at which to perform the check
            timeout: maximum allowed duration (in seconds) to wait for
                     interfaces to come up

        Raises:
            RuntimeError if the timeout duration is reached and interfaces are
                still not up
        """
        start = datetime.now()

        while (datetime.now() - start).seconds < timeout:
            if self.all_portchannels_established():
                logger.log_info("All portchannel intfs are established")
                return None
            logger.log_info("Not all portchannel intfs are established")
            time.sleep(interval)

        raise RuntimeError('Portchannel intfs were not established '
                           'within {}'.format(timeout))

    def get_ipinip_tunnel_addrs(self):
        """
        Get the IP addresses used for the IPinIP tunnel

        These should be the Loopback0 addresses for this device and the
        peer device

        Returns:
            ((str) self_loopback_ip, (str) peer_loopback_ip)
            or
            (None, None) If the tunnel type is not IPinIP
                         or
                         if an error is encountered. This most likely means
                         the host device is not a dual ToR device
        """
        try:
            peer_switch = self.config_db.get_keys(PEER_SWITCH_TABLE)[0]
            tunnel = self.config_db.get_keys(TUNNEL_TABLE)[0]
        except IndexError:
            logger.log_warning('PEER_SWITCH or TUNNEL table '
                               'not found in config DB')
            return None, None

        try:
            tunnel_table = self.config_db.get_entry(TUNNEL_TABLE, tunnel)
            tunnel_type = tunnel_table[TUNNEL_TYPE_KEY].lower()
            self_loopback_ip = tunnel_table[DST_IP_KEY]
            peer_loopback_ip = self.config_db.get_entry(
                                PEER_SWITCH_TABLE, peer_switch
                                )[ADDRESS_IPV4_KEY]
        except KeyError as error:
            logger.log_warning(
                'PEER_SWITCH or TUNNEL table missing data, '
                'could not find key {}'
                .format(error)
            )
            return None, None

        if tunnel_type == IPINIP_TUNNEL:
            return self_loopback_ip, peer_loopback_ip

        return None, None

    def get_inner_pkt_type(self, packet):
        """
        Get the type of an inner encapsulated packet

        Returns:
            (str)  'v4' if the inner packet is IPv4
            (str)  'v6' if the inner packet is IPv6
            (bool) False if `packet` is not an IPinIP packet
        """
        if packet.haslayer(IP):
            # Determine inner packet type based on IP protocol number
            # The outer packet type should always be IPv4
            if packet[IP].proto == 4:
                return IP
            elif packet[IP].proto == 41:
                return IPv6
        return False

    def sniffer_restart_required(self, lag, fvs):
        """
        Determines if the packet sniffer needs to be restarted

        The sniffer needs to be restarted when a portchannel interface transitions
        from down to up. When a portchannel interface goes down, the sniffer is
        able to continue sniffing on other portchannels. 
        """
        oper_status = dict(fvs).get(OPER_STATUS_KEY)
        if lag not in self.sniff_intfs and oper_status == 'up':
            logger.log_info('{} came back up, sniffer restart required'
                            .format(lag))
            # Don't need to modify self.sniff_intfs here since it is repopulated
            # by self.get_up_portchannels()
            return True
        elif lag in self.sniff_intfs and oper_status == 'down':
            # A portchannel interface went down, remove it from the list of
            # sniffed interfaces so we can detect when it comes back up
            self.sniff_intfs.remove(lag)
            return False
        else:
            return False

    def start_sniffer(self):
        """
        Starts an AsyncSniffer and waits for it to inititalize fully
        """
        start = datetime.now()
        
        self.sniff_intfs = self.get_up_portchannels()

        while not self.sniff_intfs:
            logger.log_info('No portchannels are up yet...')
            if (datetime.now() - start).seconds > 180:
                logger.log_error('All portchannels failed to come up within 3 minutes, exiting...')
                sys.exit(1)
            self.sniff_intfs = self.get_up_portchannels()
            time.sleep(10)

        self.sniffer = AsyncSniffer(
            iface=list(self.sniff_intfs),
            filter=self.packet_filter,
            prn=self.ping_inner_dst,
            store=0
        )
        self.sniffer.start()

        while not hasattr(self.sniffer, 'stop_cb'):
            time.sleep(0.1)

    def write_count_to_db(self):
        while True:
            # use a set to automatically deduplicate destination IPs
            to_run = set()

            to_run.add(tuple(self.pending_cmds.get()))
            pkt_count = 1
            while not self.pending_cmds.empty() and len(to_run) < 100:
                to_run.add(tuple(self.pending_cmds.get()))
                # we should always count each packet, but only ping for each unique IP
                pkt_count += 1

            for cmds in to_run:
                logger.log_info("Running command '{}'".format(' '.join(cmds)))
                subprocess.run(cmds, stdout=subprocess.DEVNULL)
            try:
                curr_count = int(self.counters_db.get(COUNTERS_DB, self.tunnel_counter_table, COUNTER_KEY))
            except TypeError:
                curr_count = 0
            self.counters_db.set(COUNTERS_DB, self.tunnel_counter_table, COUNTER_KEY, str(curr_count + pkt_count))

    def ping_inner_dst(self, packet):
        """
        Pings the inner destination IP for an encapsulated packet

        Args:
            packet: The encapsulated packet received
        """
        inner_packet_type = self.get_inner_pkt_type(packet)
        if inner_packet_type and packet[IP].dst == self.self_ip:
            cmds = ['timeout', '0.2', 'ping', '-c1',
                    '-W1', '-i0', '-n', '-q']
            if inner_packet_type == IPv6:
                cmds.append('-6')
            dst_ip = packet[IP].payload[inner_packet_type].dst
            cmds.append(dst_ip)
            self.pending_cmds.put(cmds)

    def listen_for_tunnel_pkts(self):
        """
        Listens for tunnel packets that are trapped to CPU

        These packets may be trapped if there is no neighbor info for the
        inner packet destination IP in the hardware.
        """
        self.self_ip, peer_ip = self.get_ipinip_tunnel_addrs()
        if self.self_ip is None or peer_ip is None:
            logger.log_notice('Could not get tunnel addresses from '
                              'config DB, exiting...')
            return None

        self.packet_filter = 'host {} and host {}'.format(self.self_ip, peer_ip)
        logger.log_notice('Starting tunnel packet handler for {}'
                          .format(self.packet_filter))

        app_db = DBConnector(APPL_DB, 0)
        lag_table = SubscriberStateTable(app_db, LAG_TABLE)
        sel = Select()
        sel.addSelectable(lag_table)

        self.start_sniffer()
        logger.log_info("Listening on interfaces {}".format(self.sniff_intfs))
        while True:
            rc, _ = sel.select(SELECT_TIMEOUT)

            if rc == Select.TIMEOUT:
                continue
            elif rc == Select.ERROR:
                raise Exception("Select() error")
            else:
                lag, _, fvs = lag_table.pop()
                if self.sniffer_restart_required(lag, fvs):
                    self.sniffer.stop()
                    start = datetime.now()
                    # wait up to 3 seconds for the kernel interface to be synced with APPL_DB status
                    while (datetime.now() - start).seconds < 3:
                        self.sniff_intfs = self.get_up_portchannels()
                        if lag in self.sniff_intfs:
                            break
                        time.sleep(0.1)
                    logger.log_notice('Restarting tunnel packet handler on '
                                    'interfaces {}'.format(self.sniff_intfs))
                    self.start_sniffer()

    def run(self):
        """
        Entry point for the TunnelPacketHandler class
        """
        self.wait_for_portchannels()
        db_thread = Thread(target=self.write_count_to_db, daemon=True)
        db_thread.start()
        self.listen_for_tunnel_pkts()


def main():
    logger.set_min_log_priority_info()
    handler = TunnelPacketHandler()
    handler.run()


if __name__ == "__main__":
    main()