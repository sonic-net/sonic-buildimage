#!/usr/bin/env python

from scapy.all import *
import socket
from threading import Thread
from sonic_py_common import daemon_base, logger
import time
from swsscommon import swsscommon
import netifaces
from ipaddress import ip_interface, ip_network
from pyroute2 import IPRoute
from pyroute2.netlink.exceptions import NetlinkError
from datetime import datetime


SYSLOG_IDENTIFIER = "vnet-monitor"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)

DEFAULT_INTERVAL = 5000 # The default ping interval is set to 5000 ms
DEFAULT_MULTIPLIER = 3  # The default multipler is set to 3
DEFAULT_VNI = 8000        # The default vni
DEFAULT_VXLAN_UDP_PORT = 65330   # The default Vxlan port
DEFAULT_UDP_PORT = 10000 # The default inner packet UDP port
DEFAULT_DSCP = 48 # The default DSCP value in IP header of vnet ping
TRUSTED_VNI = 256 # The trusted VNI by Pensando card
TRUSTED_SPORT = 64128 # The trusted source port by Pensando card
TIMEOUT_CHECK_INTERVAL = 1000 # The interval (in ms) to check the timeout event

# The state for current ping
PING_INITIAL = 0 # Initial state, ping request sent, no reply yet and not timeout
PING_TIMEOUT = 1 # The current ping is timeout
PING_REPLIED = 2 # The current gets reply

# Dict key name
KEY_INTERVAL = "interval"
KEY_MULTIPLIER = "multiplier"
KEY_T0_LO = "t0_lo"
KEY_CARD_VIP = "card_vip"
KEY_SEQ_NUM = "seq_num"
KEY_STATE = "state"
KEY_OVERLAY_MAC = "overlay_mac"
KEY_PACKET_TMPL = "packet_tmpl"
KEY_VNI = "vni"
KEY_TIMEOUT_COUNT = "timeout_count"
KEY_LAST_TIMEOUT_CHECK = "last_timeout_check"
KEY_LAST_RESPONSE = "last_response"
KEY_LAST_PING = "last_ping"
KEY_PING_STATE = "ping_state"

# Table and key name in LAG_TABLE
LAG_TABLE = 'LAG_TABLE'
OPER_STATUS_KEY = 'oper_status'

# Increase recv buffer size to workaround packet drop issue
# The default buffer size is 65536 bytes
conf.bufsize *= 10

# Ping task list
g_ping_task_list = []

# TSA state
is_state_TSA = False

def ip_addr(ip_network_addr):
    """
    A helper function to get the IP address from an IP address with mask
    return a string
    """
    return str(ip_network(ip_network_addr, strict=False).network_address)

def monotonic_ms():
    return time.monotonic_ns()/1000000


class TaskQueue:
    """
    A class to hold all the tasks and the status of tasks
    """
    def __init__(self):
        pass
        
class PingStatusCache:
    """
    A class to cache the ping result
    """

    def __init__(self):
        self.stats = {}
        self.vnet_monitor_table = None
        self.lock = threading.Lock()
        self.working = True
        self.worker_thread = Thread(target=self.timeout_check_thread)
        self.worker_thread.start()
    
    def teardown(self):
        """
        Stop the timeout check thread
        """
        self.working = False
        if self.worker_thread is not None:
            self.worker_thread.join(timeout=30)
        self.stats.clear()
    
    def make_key(self, t0_loopback, card_vip):
        """
        Get the key for a given t0_lo and VIP combination
        """
        return (ip_addr(t0_loopback), ip_addr(card_vip))

    def set_state_db_table(self, vnet_monitor_table):
        self.vnet_monitor_table = vnet_monitor_table

    def add_entry(self, t0_loopback, card_vip, interval, multiplier, overlay_mac):
        """
        Add an entry in the cache
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            interval: The ping interval
            multiplier: The ping timeout multiplier
            overlay_mac: The overlay MAC address
        Returns:
            None
        Raises:
            None
        """
        # The key in cache is without prefix length
        key = self.make_key(t0_loopback, card_vip)
        with self.lock:
            if key in self.stats:
                # Update the existing cache
                self.stats[key][KEY_INTERVAL] = interval
                self.stats[key][KEY_MULTIPLIER] = multiplier
                self.stats[key][KEY_OVERLAY_MAC] = overlay_mac
            else:
                self.stats[key] = {
                    KEY_T0_LO: t0_loopback,
                    KEY_CARD_VIP: card_vip,
                    KEY_SEQ_NUM: 0,
                    KEY_TIMEOUT_COUNT: 0,
                    KEY_STATE: "down",
                    KEY_PING_STATE: PING_INITIAL,
                    KEY_INTERVAL: interval,
                    KEY_MULTIPLIER: multiplier,
                    KEY_OVERLAY_MAC: overlay_mac,
                    KEY_LAST_RESPONSE: 0,
                    KEY_LAST_TIMEOUT_CHECK: 0
                }
                if self.vnet_monitor_table is not None:
                    # Also create entry in STATE_DB if not found
                    table_key = t0_loopback + "|" + card_vip
                    ret, state = self.vnet_monitor_table.hget(table_key, "state")
                    if not ret:
                        kvs = [("state", "down")]
                        try:
                            self.vnet_monitor_table.set(table_key, kvs)
                        except RuntimeError as e:
                            logger_helper.log_notice("Exception caught {}".format(e))
                            pass
                    else:
                        self.stats[key]['state'] = state
    
    def remove_entry(self, t0_loopback, card_vip):
        """
        Remove an entry from the cache. Do nothing if the entry is not found
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
        Returns:
            None
        Raises:
            None
        """
        key = self.make_key(t0_loopback, card_vip)
        with self.lock:
            if key in self.stats:
                # Remove entry from STATE_DB as well
                table_key = self.stats[key]['t0_lo'] + "|" + self.stats[key]['card_vip']
                if self.vnet_monitor_table is not None:
                    self.vnet_monitor_table.delete(table_key)
                    logger_helper.log_notice("Removed state_db entry for loopback {} VIP {}".format(t0_loopback, card_vip))
                # Remove entry from cache
                self.stats.pop(key)

    def has_entry(self, t0_loopback, card_vip):
        """
        Check if the given entry is in cached status
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
        Returns:
            True if the entry is found, otherwise return False
        Raises:
            None
        """
        key = self.make_key(t0_loopback, card_vip)
        return key in self.stats
    
    def compare_entry(self, t0_loopback, card_vip, interval, multiplier, overlay_mac):
        """
        Compare the given entry with the cached entry
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            interval: The ping interval
            multiplier: The ping timeout multiplier
            overlay_mac: The overlay MAC address
        Returns:
            True if the entry is found and the values are the same, otherwise return False
        Raises:
            None
        """
        key = self.make_key(t0_loopback, card_vip)
        if not self.has_entry(t0_loopback, card_vip):
            return False
        return self.stats[key]['interval'] == interval and \
            self.stats[key]['multiplier'] == multiplier and \
            self.stats[key]['overlay_mac'] == overlay_mac
    
    def update_sequence_number(self, t0_loopback, card_vip, seq_num=None):
        """
        Read the sequence number in cache
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            seq_num: Set the sequence number to seq_num if not None, else advance by 1
        Returns:
            None
        Raises:
            None 
        """
        with self.lock:
            if not self.has_entry(t0_loopback, card_vip):
                return -1
            key = self.make_key(t0_loopback, card_vip)
            if seq_num is not None:
                self.stats[key][KEY_SEQ_NUM] = seq_num
            else:
                self.stats[key][KEY_SEQ_NUM] += 1
            # Since the sequence number increased, we need to reset the ping state
            self.stats[key][KEY_PING_STATE] = PING_INITIAL

    def vnet_state_update(self, t0_loopback, card_vip, state):
        """
        Update the state of given vet in both cache and state_db table
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            state: The current state(down/up). The function does nothing if the existing state is the same as the state to set
        Returns:
            None
        Raises:
            None
        """
        if state not in ['down', 'up']:
            logger_helper.log_warning("Invalid state {} string for t0 {} VIP {}", state, t0_loopback, card_vip)
            return
        # Only update if the cached state is changed
        key = self.make_key(t0_loopback, card_vip)
        current_state = self.stats[key]['state']
        if current_state != state:
            self.stats[key]['state'] = state
            table_key = self.stats[key]['t0_lo'] + "|" + self.stats[key]['card_vip']
            kvs = [("state", state)]
            if self.vnet_monitor_table is not None:
                logger_helper.log_notice("VNET t0 {} VIP {} state transit from {} to {}".format(t0_loopback, ip_addr(card_vip), current_state, state))
                try:
                    self.vnet_monitor_table.set(table_key, kvs)
                except RuntimeError as e:
                    logger_helper.log_notice("Exception caught {}".format(e))
                    pass
    
    def set_timeout_state(self, t0_loopback, card_vip):
        """
        Increase the timeout count of given entry. Do nothing if the entry is not found
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
        Returns:
            Return the timeout_count after adding 1.
        Raises:
            None 
        """
        with self.lock:
            if not self.has_entry(t0_loopback, card_vip):
                return 0
            key = self.make_key(t0_loopback, card_vip)
            # Set the ping state
            self.stats[key][KEY_PING_STATE] = PING_TIMEOUT
            # Increase the timeout count
            self.stats[key][KEY_TIMEOUT_COUNT] += 1
            if self.stats[key][KEY_TIMEOUT_COUNT] >= self.stats[key][KEY_MULTIPLIER]:
                self.vnet_state_update(t0_loopback, card_vip, "down")
                self.stats[key][KEY_TIMEOUT_COUNT] = 0
            return self.stats[key][KEY_TIMEOUT_COUNT]
    
    def set_up_state(self, t0_loopback, card_vip, seq_num):
        """
        Call this function when reply is seen.
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            seq_num: The sequence number in the ping reply
        Returns:
            None
        Raises:
            None 
        """
        with self.lock:
            if not self.has_entry(t0_loopback, card_vip):
                return
            key = self.make_key(t0_loopback, card_vip)
            # Got an old reply
            if seq_num + self.stats[key][KEY_MULTIPLIER] < self.stats[key][KEY_SEQ_NUM]:
                return
            # Update the ping state
            self.stats[key][KEY_PING_STATE] = PING_REPLIED
            # Update the sequence number in cache
            self.stats[key][KEY_SEQ_NUM] = seq_num + 1
            # Clear the timeout count
            self.stats[key][KEY_TIMEOUT_COUNT] = 0
            # Update the last response time
            self.stats[key][KEY_LAST_RESPONSE] = monotonic_ms()
            self.vnet_state_update(t0_loopback, card_vip, "up")

    def timeout_check_thread(self):
        while self.working:
            timeout_check_interval = TIMEOUT_CHECK_INTERVAL
            cur_time = monotonic_ms()
            for stat in list(self.stats.values()):
                if cur_time - stat[KEY_LAST_TIMEOUT_CHECK] >= stat[KEY_INTERVAL]:
                    stat[KEY_LAST_TIMEOUT_CHECK] = cur_time
                    if cur_time - stat[KEY_LAST_RESPONSE] > stat[KEY_INTERVAL]:
                        self.set_timeout_state(stat[KEY_T0_LO], stat[KEY_CARD_VIP])
                # Use the minimum interval to check the timeout
                timeout_check_interval = min(timeout_check_interval, stat[KEY_INTERVAL])
            time.sleep(timeout_check_interval/1000)


class TaskPing():
    
    # We create only 1 fd for all ping tasks
    # The fd is shared among all ping tasks. It's safe because all ping tasks
    # are running sequentially
    socket_ping = None

    def __init__(self, task_queue, t1_mac, t1_loopback_v4, t1_loopback_v6):
        """
        Constructor of TaskPing
        Args:
            task_queue: A queue holding all the ping tasks and status
            t1_mac: MAC address of this T1
            t1_loopback_v4: The loopback address (IPv4) of this T1. 
            t1_loopback_v6: The loopback address (IPv6) of this T1.
        Returns:
            None
        Raises:
            None
        """
        self.task_queue = task_queue
        self.t1_mac = t1_mac
        self.t1_loopback_v4 = t1_loopback_v4
        self.t1_loopback_v6 = t1_loopback_v6
        self.socket_ping = None
        self.running = False
        self.worker_thread = None
    
    def init(self):
        """
        Create the shared socket
        Args:
            None
        Returns:
            None
        Raises:
            None
        """
        if self.socket_ping is None:
            self.socket_ping = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket_ping.setsockopt(socket.SOL_IP, socket.IP_TOS, (DEFAULT_DSCP << 2))
            TIMEOUT = 30
            while TIMEOUT > 0:
                try:
                    self.socket_ping.bind((self.t1_loopback_v4, 0))
                    break
                except OSError:
                    # The loopback address is not configured yet
                    TIMEOUT -= 1
                    time.sleep(1)
        self.running = True
        self.worker_thread = Thread(target=self.ping_thread)
        self.worker_thread.start()

    def teardown(self):
        """
        Close the shared socket
        Args:
            None
        Returns:
            None
        Raises:
            None
        """
        self.running = False
        if self.worker_thread is not None:
            self.worker_thread.join(timeout=30)
        if self.socket_ping is not None:
            self.socket_ping.close()
            self.socket_ping = None

    
    def build_vent_ping_template_packet(self, t1_mac, t1_loopback, overlay_mac, card_vip, vni):
        """
        Build a vxlan encapsulated template packet for later use.
        A payload will be added to the template to generate a final packet.
        The template is as below:
        +------+------+------+-------------------+------+-------+------+-------+------------+--------+-----+-------+
        |T1 lo |T0 lo |Vxlan1|'00:12:34:56:78:9a'|T1 MAC| T1 lo | VIP  |Vxlan2 |Special MAC | T1 MAC | VIP | T1 Lo |
        +------+------+------+-------------------+--------------+------+------_+------------+--------+-----+-------+
        Notes:
            1. The dst_mac after the 1st vxlan header is a fixed value '00:12:34:56:78:9a', otherwise the packet is dropped by T0
            2. The src_mac after the 2nd vxlan must be the special_mac. The card is reading the src_mac and do the matching
            3. The dst_mac after the 2nd vxlan header can be any value. The card will rewrite it. We use the MAC of T1 here
            4. The VXLAN flags is set to 0x08 explicitly. Otherwise the packet is dropped by T0
        Args:
            t1_mac: MAC address of this T1
            t1_loopback: The loopback address (IPV4) of this T1
            overlay_mac: The special MAC for matching the Vnet ping
            card_vip: The virtual IP address of card. The VIP of card can be IPv4 or IPv6
            vni: The vni in vxlan header
        Returns:
            packet tempalte
        Raises:
        """
        T0_MAC = "00:12:34:56:78:9a"
        if ip_interface(card_vip).version == 4:
            # The VIP of card is IPv4
            inner_pkt = Ether(dst=t1_mac, src=overlay_mac)/IP(src=card_vip, dst=t1_loopback, tos=(DEFAULT_DSCP << 2))/UDP(sport=DEFAULT_UDP_PORT, dport=DEFAULT_UDP_PORT)
            vxlan2_pkt = Ether(dst=T0_MAC, src=t1_mac)/IP(src=t1_loopback, dst=card_vip, ttl=2, tos=(DEFAULT_DSCP << 2))/UDP(sport=TRUSTED_SPORT, dport=DEFAULT_VXLAN_UDP_PORT)/VXLAN(flags=0x08, vni=TRUSTED_VNI)/inner_pkt
        else:
            # The VIP of card is IPv6
            inner_pkt = Ether(dst=t1_mac, src=overlay_mac)/IPv6(src=card_vip, dst=t1_loopback, tc=(DEFAULT_DSCP << 2))/UDP(sport=DEFAULT_UDP_PORT, dport=DEFAULT_UDP_PORT)
            vxlan2_pkt = Ether(dst=T0_MAC, src=t1_mac)/IPv6(src=t1_loopback, dst=card_vip, hlim=2, tc=(DEFAULT_DSCP << 2))/UDP(sport=TRUSTED_SPORT, dport=DEFAULT_VXLAN_UDP_PORT)/VXLAN(flags=0x08, vni=TRUSTED_VNI)/inner_pkt

        vxlan1_pkt = VXLAN(flags=0x08, vni=vni)/vxlan2_pkt

        return vxlan1_pkt

    def build_vnet_ping_packet(self, task):
        """
        Build a double vxlan encapsulated packet
           +------+------+------+-------------------+------+-------+------+-------+------------+--------+-----+-------+-------+---------+
            |T1 lo |T0 lo |Vxlan1|'00:12:34:56:78:9a'|T1 MAC| T1 lo | VIP  |Vxlan2 |Special MAC | T1 MAC | VIP | T1 Lo | T0 Lo | Seq No. |
            +------+------+------+-------------------+--------------+------+------_+------------+--------+-----+-------+-------+---------+
        Returns:
            The vnet ping packet
        Raises:
            None
        """
        if task.get(KEY_PACKET_TMPL, None) is None:
            task[KEY_PACKET_TMPL] = self.build_vent_ping_template_packet(self.t1_mac,
                                        self.t1_loopback_v4 if ip_interface(task[KEY_CARD_VIP]).version == 4 else self.t1_loopback_v6,
                                        task[KEY_OVERLAY_MAC],
                                        ip_addr(task[KEY_CARD_VIP]),
                                        task[KEY_VNI])
        pkt = None
        # The sequence number is 8 bytes in big endian.
        # If a ping packet is sent out every 1 millisecond, it will take 584942417355 years to wrap around
        load_seq_num = task[KEY_SEQ_NUM].to_bytes(8, 'big')
        load_t0_loopback = socket.inet_aton(task[KEY_T0_LO])
        load = b''.join([load_t0_loopback, load_seq_num])
        pkt = task[KEY_PACKET_TMPL]/Raw(load)
        return pkt
    
    def do_task(self, task):
        """
        Build a double vxlan encapsulated packet, and send out with scapy
        Returns:
            None
        Raises:
            None
        """
        pkt = self.build_vnet_ping_packet(task)
        if self.socket_ping is not None:
            self.socket_ping.sendto(bytes(pkt), (task[KEY_T0_LO], DEFAULT_VXLAN_UDP_PORT))
    
    def ping_thread(self):
        """
        The thread function to send out the ping packet
        """
        while self.running:
            ping_wait_time = DEFAULT_INTERVAL
            if not is_state_TSA:
                for task in self.task_queue:
                    cur_time = monotonic_ms()
                    if cur_time - task[KEY_LAST_PING] >= task[KEY_INTERVAL]:
                        self.do_task(task)
                        task[KEY_SEQ_NUM] += 1
                        task[KEY_LAST_PING] = cur_time
                        g_ping_stats.update_sequence_number(task[KEY_T0_LO], task[KEY_CARD_VIP], task[KEY_SEQ_NUM])
                    ping_wait_time = min(ping_wait_time, task[KEY_INTERVAL] - (cur_time - task[KEY_LAST_PING]))
            time.sleep(ping_wait_time/1000)

class DBMonitor():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self, table_name, tsa_table_name, t1_mac, t1_loopback, vni, cached_stats, task_list):
        self.working = False
        self.appl_db = daemon_base.db_connect("APPL_DB")
        self.config_db = daemon_base.db_connect("CONFIG_DB")
        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, table_name)
        self.sel.addSelectable(self.tbl)
        self.tsa_tbl = swsscommon.SubscriberStateTable(self.config_db, tsa_table_name)
        self.sel.addSelectable(self.tsa_tbl)
        self.t1_mac = t1_mac
        self.t1_loopback = t1_loopback
        self.vni = vni
        self.cached_stats = cached_stats
        self.task_list = task_list

    def process_new_entry(self, key, op, fvp):
        if not op in ['SET', 'DEL']:
            return
        delimiter = ':'
        keys = key.split(delimiter, 1)
        if len(keys) != 2:
            logger_helper.log_warning("Got invalid vnet ping task. Key: {}".format(key))
            return
        #Key format VNET_MONITOR_TABLE:T0_lo:VIP. The VIP can be both IPv4 and IPv6
        t0_loopback = keys[0]
        card_vip = keys[1]
        try:
            if ip_interface(t0_loopback).version != 4:
                logger_helper.log_warning("T0 Loopback should be IPv4 address: {}".format(t0_loopback))
                return
            af = ip_interface(card_vip).version
        except ValueError:
            logger_helper.log_warning("Got invalid vnet ping task. Key: {}".format(key))
            return
        if op == 'SET':
            interval = int(fvp.get("interval", DEFAULT_INTERVAL))
            multiplier = int(fvp.get("multiplier", DEFAULT_MULTIPLIER))
            overlay_mac = fvp.get("overlay_dmac")
            if self.cached_stats.has_entry(t0_loopback, card_vip):
                if self.cached_stats.compare_entry(t0_loopback, card_vip, interval, multiplier, overlay_mac):
                    # The entry is already in the cache, and the new entry is the same as the cached one
                    return
                logger_helper.log_notice("Update existing vnet ping task T0 Loopback: {} VIP: {}".format(t0_loopback, card_vip))

            logger_helper.log_notice("Got new vnet ping task T0 Loopback: {} VIP: {}".format(t0_loopback, card_vip))
            self.cached_stats.add_entry(t0_loopback, card_vip, interval, multiplier, overlay_mac)
            for task in self.task_list:
                # Update existing task
                if task[KEY_T0_LO] == t0_loopback and task[KEY_CARD_VIP] == card_vip:
                    task[KEY_INTERVAL] = interval
                    task[KEY_MULTIPLIER] = multiplier
                    task[KEY_OVERLAY_MAC] = overlay_mac
                    task[KEY_LAST_PING] = 0
                    break
            else:
                # Create new task
                self.task_list.append(
                    {
                        KEY_T0_LO: t0_loopback,
                        KEY_CARD_VIP: card_vip,
                        KEY_OVERLAY_MAC: overlay_mac,
                        KEY_VNI: self.vni,
                        KEY_INTERVAL: interval,
                        KEY_MULTIPLIER: multiplier,
                        KEY_SEQ_NUM: 0,
                        KEY_LAST_PING: 0
                    }
                )
        else:
            logger_helper.log_notice("Removed vnet ping task T0 Loopback: {} VIP: {}".format(t0_loopback, card_vip))
            self.cached_stats.remove_entry(t0_loopback, card_vip)
            for task in self.task_list:
                if task[KEY_T0_LO] == t0_loopback and task[KEY_CARD_VIP] == card_vip:
                    self.task_list.remove(task)
                    break


    def start(self):
        """
        Start monitoring the table
        """
        self.working = True
        SELECT_TIMEOUT_MSECS = 1000
        global is_state_TSA
        while True:
            (state, selectableObj) = self.sel.select(SELECT_TIMEOUT_MSECS)
            if not self.working:
                break
            if state == swsscommon.Select.OBJECT:
                if selectableObj.getFd() == self.tbl.getFd():
                    (key, op, fvp) = self.tbl.pop()
                    self.process_new_entry(key, op, dict(fvp))
                elif selectableObj.getFd() == self.tsa_tbl.getFd():
                    (_, _, fvp) = self.tsa_tbl.pop()
                    tsa_state = dict(fvp).get("tsa_enabled", "false")
                    if tsa_state.lower() == "true":
                        is_state_TSA = True
                    else:
                        is_state_TSA = False
                    logger_helper.log_notice("TSA state changed to {}".format(is_state_TSA))

    def stop(self):
        """
        Stop the select operation and exit
        """
        self.working = False

g_ping_stats = PingStatusCache()

def process_packet(packet):
    # packet is assumed to be a UDP packet with custom payload
    try:
        load = packet[Raw].load
        t0_loopback = socket.inet_ntoa(load[0:4])
        seq_number = int.from_bytes(load[4:12], 'big')
        if IP in packet:
            vip = packet[IP].src
        elif IPv6 in packet:
            vip = packet[IPv6].src
        else:
            return
    except IndexError:
        return
    logger_helper.log_debug("Got vnet ping reply t0_loopback {} vip {} seq_num {}".format(t0_loopback, vip, seq_number))
    g_ping_stats.set_up_state(t0_loopback, vip, seq_number)


class ReplyMonitor():
    """
    A class to monitor the vnet ping reply
    """
    def __init__(self, t1_loopback, port):
        self.loopback = t1_loopback
        self.port = port
        self.sniffstring = "udp and (dst host " + str(self.loopback[4]) + " or dst host " + self.loopback[6] +  ") and dst port " + str(self.port)
        self.watching_thread = None
        self.sniffer = None
        self.working = False
        self.sniffing_iface = []
        self.netlink_api = IPRoute()


    def _get_bgp_local_address(self):
        """
        Read the local address of bgp peers from CONFIG_DB.
        Return a list of local address
        """
        local_addresses = []
        config_db = swsscommon.ConfigDBConnector()
        config_db.connect()
        data = config_db.get_table('BGP_NEIGHBOR')

        for neighbor_ip in data.keys():
            if ip_interface(neighbor_ip).version != 4:
                continue
            local_addresses.append(data[neighbor_ip]['local_addr'])
        
        return local_addresses


    def _get_iface_list(self):
        """
        Get the interface list including both up and down
        """
        ifaces = []
        iface_list = netifaces.interfaces()
        bgp_local_addresses = self._get_bgp_local_address()
        for iface in iface_list:
            try:
                if netifaces.ifaddresses(iface)[netifaces.AF_INET][0]['addr'] in bgp_local_addresses:
                    ifaces.append(iface)
            except KeyError:
                pass
            except ValueError:
                pass

        return ifaces


    def _get_oper_up_iface_list(self):
        """
        Get an list of operation up interfaces.
        """
        ifaces = self._get_iface_list()
        up_ifaces = []
        for iface in ifaces:
            try:
                status = self.netlink_api.link("get", ifname=iface)
            except NetlinkError:
                continue
        
            if status[0]['state'] == 'up':
                up_ifaces.append(iface)
        return up_ifaces


    def sniffer_restart_required(self, lag, fvs):
        """
        Determines if the packet sniffer needs to be restarted

        The sniffer needs to be restarted when a portchannel interface transitions
        from down to up. When a portchannel interface goes down, the sniffer is
        able to continue sniffing on other portchannels. 
        """
        oper_status = dict(fvs).get(OPER_STATUS_KEY)
        if lag not in self.sniffing_iface and oper_status == 'up':
            logger_helper.log_info('{} came back up, sniffer restart required'
                            .format(lag))
            # Don't need to modify self.sniffing_iface here since it is repopulated
            # by self.get_up_portchannels()
            return True
        elif lag in self.sniffing_iface and oper_status == 'down':
            # A portchannel interface went down, remove it from the list of
            # sniffed interfaces so we can detect when it comes back up
            self.sniffing_iface.remove(lag)
            return False
        else:
            return False


    def _get_intf_name(self, msg):
        """
        Read the intf name from the link message
        """
        attr_list = msg.get('attrs', list())
        for attribute, val in attr_list:
            if attribute == 'IFLA_IFNAME':
                return val

        return ''

    
    def _sniffer_watching_thread(self):
        """
        A thread to watch the link up message. Restart the sniffer if the msg is seen
        """
        app_db = swsscommon.DBConnector("APPL_DB", 0)
        lag_table = swsscommon.SubscriberStateTable(app_db, LAG_TABLE)
        sel = swsscommon.Select()
        sel.addSelectable(lag_table)
        SELECT_TIMEOUT_MSECS = 1000
        logger_helper.log_info('Starting LAG state watching thread')
        while self.working:
            rc, _ = sel.select(SELECT_TIMEOUT_MSECS)
            if rc == swsscommon.Select.TIMEOUT:
                continue
            elif rc == swsscommon.Select.ERROR:
                raise Exception("Select() error")
            else:
                lag, _, fvs = lag_table.pop()
                if self.sniffer_restart_required(lag, fvs):
                    TIMEOUT = 10
                    up_count = 0
                    THRESHOLD = 5
                    # Restart sniff only when the newly up LAG keeps up for over 5 seconds
                    start = datetime.now()
                    while (datetime.now() - start).seconds < TIMEOUT:
                        if lag in self._get_oper_up_iface_list():
                            up_count += 1
                            if up_count >= THRESHOLD:
                                break
                        else:
                            up_count = 0
                        time.sleep(1)
                        
                    # Restart the sniffer if there is no interface flap
                    if up_count >= THRESHOLD:
                        self.restart_sniff()


    def start(self):
        """
        Start the async sniffer and the watching thread
        """
        self.start_sniff()
        self.watching_thread = Thread(target=self._sniffer_watching_thread)
        self.watching_thread.start()


    def start_sniff(self):
        if self.sniffer is not None:
            return
        self.sniffing_iface = self._get_oper_up_iface_list()
        while self.sniffing_iface == []:
            # Wait for at least one interface is up, oterwise the sniffer will not work
            time.sleep(1)
            self.sniffing_iface = self._get_oper_up_iface_list()
        while True:
            self.sniffer = AsyncSniffer(iface=self.sniffing_iface, filter=self.sniffstring, prn=process_packet, store=False)
            self.sniffer.start()
            # Allow 3 seconds for the sniffer to be fully initialized
            # If the sniffer is not fully initialized, then it could because some exception happened
            # Then we need to restart the sniffer
            RETRY = 30
            while RETRY >= 0 and not hasattr(self.sniffer, 'stop_cb'):
                time.sleep(0.1)
                RETRY -= 1
            if RETRY < 0:
                logger_helper.log_warning("Failed to start sniffer, retrying")
                time.sleep(1)
                self.sniffing_iface = self._get_oper_up_iface_list()
            else:
                break

        logger_helper.log_notice("Started vnet ping monitor at interfaces {}".format(self.sniffing_iface))
        self.working = True
        

    def restart_sniff(self):
        """
        Restart the sniffer
        """
        if self.sniffer is None or not self.working:
            return
        if self.sniffing_iface == self._get_oper_up_iface_list():
            # Ignore the restart request if no change in oper up interface
            return
        try:
            self.sniffer.stop()
        except:
            # An exception is thrown if sniffer is not fully started
            # Ignore it
            pass
        self.sniffer = None
        self.working = False
        logger_helper.log_notice("Going to restart vnet ping monitor")
        self.start_sniff()


    def stop(self):
        if self.sniffer == None:
            return
        self.working = False
        self.sniffer.stop()
        self.sniffer = None
        self.watching_thread.join()

