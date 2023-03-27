#!/usr/bin/env python

from scapy.all import *
import socket
from threading import Thread
from sonic_py_common import daemon_base, logger
from queue import PriorityQueue, Empty
import time
import random
from swsscommon import swsscommon
import netifaces
from ipaddress import ip_interface, ip_network
from pyroute2 import IPRoute
from pyroute2.netlink.exceptions import NetlinkError


SYSLOG_IDENTIFIER = "vnet-monitor"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)

DEFAULT_INTERVAL = 1000 # The default ping interval is set to 1000 ms
DEFAULT_MULTIPLIER = 3  # The default multipler is set to 3
DEFAULT_VNI = 8000        # The default vni
DEFAULT_VXLAN_UDP_PORT = 4789   # The default Vxlan port
DEFAULT_UDP_PORT = 10000 # The default inner packet UDP port
DEFAULT_DSCP = 48 # The default DSCP value in IP header of vnet ping


# The state for current ping
PING_INITIAL = 0 # Initial state, ping request sent, no reply yet and not timeout
PING_TIMEOUT = 1 # The current ping is timeout
PING_REPLIED = 2 # The current gets reply

def ip_addr(ip_network_addr):
    """
    A helper function to get the IP address from an IP address with mask
    return a string
    """
    return str(ip_network(ip_network_addr).network_address)

def monotonic_ms():
    return time.monotonic_ns()/1000000

class PingStatusCache:
    """
    A class to cache the ping result
    """

    def __init__(self):
        self.stats = {}
        self.vnet_monitor_table = None
        self.lock = threading.Lock()
    
    def make_key(self, t0_loopback, card_vip):
        """
        Get the key for a given t0_lo and VIP combination
        """
        return (ip_addr(t0_loopback), ip_addr(card_vip))

    def set_state_db_table(self, vnet_monitor_table):
        self.vnet_monitor_table = vnet_monitor_table

    def add_entry(self, t0_loopback, card_vip):
        """
        Add an entry in the cache
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
        Returns:
            None
        Raises:
            None
        """
        # The key in cache is without prefix length
        key = self.make_key(t0_loopback, card_vip)
        if key in self.stats:
            return
        self.stats[key] = {
            "t0_lo": t0_loopback,
            "card_vip": card_vip,
            "seq_num": 0,
            "timeout_count": 0,
            "state": "down",
            "ping_state": PING_INITIAL
        }
        if self.vnet_monitor_table is not None:
            # Also create entry in STATE_DB if not found
            table_key = t0_loopback + "|" + card_vip
            ret, state = self.vnet_monitor_table.hget(table_key, "state")
            if not ret:
                kvs = [("state", "down")]
                self.vnet_monitor_table.set(table_key, kvs)
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
        if key in self.stats:
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
        if not self.has_entry(t0_loopback, card_vip):
            return -1
        key = self.make_key(t0_loopback, card_vip)
        if seq_num is not None:
            self.stats[key]['seq_num'] = seq_num
        else:
            self.stats[key]['seq_num'] += 1
        # Since the sequence number increased, we need to reset the ping state
        self.stats[key]['ping_state'] = PING_INITIAL

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
                logger_helper.log_notice("VNET t0 {} VIP {} state transit from {} to {}".format(t0_loopback, card_vip, current_state, state))
                self.vnet_monitor_table.set(table_key, kvs)
    
    def set_timeout_state(self, t0_loopback, card_vip, seq_num, multiplier):
        """
        Increase the timeout count of given entry. Do nothing if the entry is not found
        Args:
            t0_loopback: The loopback address (IPv4) of target T0
            card_vip: The virtual IP address (IPv4) of the card
            seq_num: The sequence number of the timeout event
            multiplier: Set state to down if multiplier timeout event received
        Returns:
            Return the timeout_count after adding 1.
        Raises:
            None 
        """
        if not self.has_entry(t0_loopback, card_vip):
            return 0
        key = self.make_key(t0_loopback, card_vip)
        if seq_num < self.stats[key]['seq_num'] or self.stats[key]['ping_state'] != PING_INITIAL:
            return 0
        with self.lock:
            # Set the ping state
            self.stats[key]['ping_state'] = PING_TIMEOUT
            # Increase the timeout count
            self.stats[key]['timeout_count'] += 1
            # Increase the sequence number in cache
            self.update_sequence_number(t0_loopback, card_vip, seq_num+1)
            if self.stats[key]['timeout_count'] >= multiplier:
                self.vnet_state_update(t0_loopback, card_vip, "down")
                self.stats[key]['timeout_count'] = 0
            return self.stats[key]['timeout_count']
    
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
        if not self.has_entry(t0_loopback, card_vip):
            return
        key = self.make_key(t0_loopback, card_vip)
        # Got an old reply
        if seq_num < self.stats[key]['seq_num'] or self.stats[key]['ping_state'] != PING_INITIAL:
            return
        with self.lock:
            # Update the ping state
            self.stats[key]['ping_state'] = PING_REPLIED
            # Update the sequence number in cache
            self.update_sequence_number(t0_loopback, card_vip, seq_num+1)
            # Clear the timeout count
            self.stats[key]['timeout_count'] = 0
            self.vnet_state_update(t0_loopback, card_vip, "up")


class TaskBase:

    def __init__(self, expected_running_timestamp_ms):
        """
        Constructor of TaskBase
        
            Args:
                expected_running_timestamp_ms: The timestamp when the task is going to be started

            Returns:
                None

            Raises:
                None
        """
        self.expected_running_timestamp_ms = expected_running_timestamp_ms
    
    def do_task(self):
        """An abstract method to do the real task"""
        pass
    

    def __lt__(self, other):
        """Override the __lt__ method inorder to put the tasks into
        a PriorityQueue"""
        return self.expected_running_timestamp_ms <= other.expected_running_timestamp_ms


class TaskPing(TaskBase):
    
    # We create only 1 fd for all ping tasks
    # The fd is shared among all ping tasks. It's safe because all ping tasks
    # are running sequentially
    socket_ping = None

    def __init__(self, expected_running_timestamp_ms, t1_mac, t1_loopback, t0_loopback, special_mac, card_vip, vni, interval_ms, multiplier, seq_num):
        """
        Constructor of TaskPing
        Args:
            expected_running_timestamp_ms: An integer, the timestamp when the task is going to be started
            t1_mac: MAC address of this T1
            t1_loopback: The loopback address (IPv4 or IPv6) of this T1. 
            t0_loopback: The loopback address (IPV4) of the target T0
            special_mac: The special MAC for matching the Vnet ping
            card_vip: The virtual IP address of card. The VIP of card can be IPv4 or IPv6
            vni: The vni in vxlan header
            interval_ms: The interval for ping request (in ms)
            multiplier: An integer. Will mark `down` in sate_db if consecutive timeout event happened
            seq_num: An integer, the sequence number of ping request
        Returns:
            None
        Raises:
            None
        """
        super().__init__(expected_running_timestamp_ms)
        self.t1_mac = t1_mac
        self.t1_loopback = t1_loopback
        self.t0_loopback = t0_loopback
        self.special_mac = special_mac
        self.card_vip = card_vip
        self.vni = vni
        self.interval_ms = interval_ms
        self.multiplier = multiplier
        self.seq_num = seq_num
        # Save the loopback address without prefix len, so that we don't
        # need to calculate it upon each send
        self.t0_lo_addr = ip_addr(self.t0_loopback)
        self.load_t0_loopback = socket.inet_aton(self.t0_lo_addr)
        self.packet_tmpl = self.build_vent_ping_template_packet(t1_mac, ip_addr(t1_loopback), special_mac, ip_addr(card_vip), vni)
    
    @staticmethod
    def init(t1_loopback_v4):
        """
        Create the shared socket
        Args:
            t1_loopback_v4: The loopback address (IPv4 only)
        Returns:
            None
        Raises:
            None
        """
        if TaskPing.socket_ping is None:
            TaskPing.socket_ping = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            TaskPing.socket_ping.setsockopt(socket.SOL_IP, socket.IP_TOS, (DEFAULT_DSCP << 2))
            TIMEOUT = 30
            while TIMEOUT > 0:
                try:
                    TaskPing.socket_ping.bind((t1_loopback_v4, 0))
                    break
                except OSError:
                    # The loopback address is not configured yet
                    TIMEOUT -= 1
                    time.sleep(1)


    @staticmethod
    def teardown():
        """
        Close the shared socket
        Args:
            None
        Returns:
            None
        Raises:
            None
        """
        if TaskPing.socket_ping is not None:
            TaskPing.socket_ping.close()
            TaskPing.socket_ping = None

    
    def build_vent_ping_template_packet(self, t1_mac, t1_loopback, special_mac, card_vip, vni):
        """
        Build a vxlan encapsulated template packet for later use.
        A payload will be added to the template to generate a final packet.
        +------+------+------+-----------+------+-------+------+-------+------------+--------+-----+-------+
        |T1 lo |T0 lo |Vxlan1|Special MAC|T1 MAC| T1 lo | VIP  |Vxlan2 |Special MAC | T1 MAC | VIP | T1 Lo |
        +------+------+------+-----------+--------------+------+------_+------------+--------+-----+-------+
        Args:
            t1_mac: MAC address of this T1
            t1_loopback: The loopback address (IPV4) of this T1
            special_mac: The special MAC for matching the Vnet ping
            card_vip: The virtual IP address of card. The VIP of card can be IPv4 or IPv6
            vni: The vni in vxlan header
        Returns:
            packet tempalte
        Raises:
        """
        if ip_interface(card_vip).version == 4:
            # The VIP of card is IPv4
            inner_pkt = Ether(dst=special_mac, src=t1_mac)/IP(src=card_vip, dst=t1_loopback, tos=(DEFAULT_DSCP << 2))/UDP(sport=DEFAULT_UDP_PORT, dport=DEFAULT_UDP_PORT)
            vxlan2_pkt = Ether(dst=special_mac, src=t1_mac)/IP(src=t1_loopback, dst=card_vip, ttl=2, tos=(DEFAULT_DSCP << 2))/UDP()/VXLAN(vni=vni)/inner_pkt
        else:
            # The VIP of card is IPv6
            inner_pkt = Ether(dst=special_mac, src=t1_mac)/IPv6(src=card_vip, dst=t1_loopback, tc=(DEFAULT_DSCP << 2))/UDP(sport=DEFAULT_UDP_PORT, dport=DEFAULT_UDP_PORT)
            vxlan2_pkt = Ether(dst=special_mac, src=t1_mac)/IPv6(src=t1_loopback, dst=card_vip, hlim=2, tc=(DEFAULT_DSCP << 2))/UDP()/VXLAN(vni=vni)/inner_pkt

        vxlan1_pkt = VXLAN(vni=vni)/vxlan2_pkt

        return vxlan1_pkt

    def build_vnet_ping_packet(self):
        """
        Build a double vxlan encapsulated packet
            +------+------+------+-----------+------+------+------+-------+------------+--------+-----+-------+-------+---------+
            |T1 lo |T0 lo |Vxlan1|Special MAC|T1 MAC|T1 lo | VIP  |Vxlan2 |Special MAC | T1 MAC | VIP | T1 Lo | T0 Lo | Seq No. |
            +------+------+------+-----------+------+------+------+------_+------------+--------+-----+-------+-------+---------+
        Returns:
            The vnet ping packet
        Raises:
            None
        """
        pkt = None
        load_seq_num = self.seq_num.to_bytes(4, 'big')
        load = b''.join([self.load_t0_loopback, load_seq_num])
        pkt = self.packet_tmpl/Raw(load)
        return pkt
    
    def do_task(self):
        """
        Build a double vxlan encapsulated packet, and send out with scapy
        Args:
            None
        Returns:
            None
        Raises:
            None
        """
        pkt = self.build_vnet_ping_packet()
        if TaskPing.socket_ping is not None:
            TaskPing.socket_ping.sendto(bytes(pkt), (self.t0_lo_addr, DEFAULT_VXLAN_UDP_PORT))
    
    def next_ping_task(self, seq_num=None):
        """
        Generate the next ping task
        """
        if seq_num is not None:
            self.seq_num = seq_num
        else:
            self.seq_num += 1
        self.expected_running_timestamp_ms = monotonic_ms() + self.interval_ms
        return self


class TaskTimeout(TaskBase):

    def __init__(self, expected_running_timestamp_ms, t0_loopback, card_vip, seq_num, multiplier):
        """
        Constructor of TaskTimeout
        Args:
            expected_running_timestamp_ms: An integer, the timestamp when the task is going to be started
            t0_loopback: The loopback address (IPV4) of the target T0
            card_vip: The virtual IP address of card
            seq_num: The sequence number for the timeout event
            multiplier: An integer. Will mark `down` in sate_db if consecutive timeout event happened
        Returns:
            None
        Raises:
            None
        """
        super().__init__(expected_running_timestamp_ms)
        self.t0_loopback = t0_loopback
        self.card_vip = card_vip
        self.seq_num = seq_num
        self.multiplier = multiplier

    def do_task(self):
        """
        Set the timeout status of given entry
        """
        logger_helper.log_debug("Vnet ping timeout on t0_loopback {} vip {}".format(self.t0_loopback, self.card_vip))


class TaskDummy(TaskBase):
    """
    A dummy task to unblock the queue.get
    """
    def __init__(self,expected_running_timestamp_ms):
        super().__init__(expected_running_timestamp_ms)

    def do_task(self):
        pass


class TaskRunner():
    """
    Run tasks in a PriorityQueue
    """

    def __init__(self, cached_stats):
        self.task_queue = PriorityQueue()
        self.running = False
        self.cached_stats = cached_stats
        self.worker_thread = None

    def add_task(self, task):
        """
        Add a task to the task_queue.
        The tasks are ordered by timestamp
        """
        self.task_queue.put(item=task, block=False)
    
    def task_runner(self):
        """
        The thread function to run each task
        """
        DIVIDEND = 1000
        while True:
            try:
                task = self.task_queue.get()
                if not self.running:
                    break
                cur_time = monotonic_ms()
                expected_time = task.expected_running_timestamp_ms
                if cur_time < expected_time:
                    time.sleep((expected_time - cur_time)/DIVIDEND)
                # Run task
                task.do_task()
                # After task running
                if isinstance(task, TaskPing):
                    # Schedule the next Timeout task
                    next_timeout_task = TaskTimeout(monotonic_ms() + task.multiplier*task.interval_ms, task.t0_loopback, task.card_vip, task.seq_num, task.multiplier)
                    self.add_task(next_timeout_task)
                    # Schedule the next ping task if cached entry is not removed
                    if self.cached_stats.has_entry(task.t0_loopback, task.card_vip):
                        next_ping_task = task.next_ping_task()
                        self.add_task(next_ping_task)
                elif isinstance(task, TaskTimeout):
                    self.cached_stats.set_timeout_state(task.t0_loopback, task.card_vip, task.seq_num, task.multiplier)
            except Empty:
                pass
    
    def start(self):
        """
        Run the worker function in a new thread
        """
        self.worker_thread = Thread(target=self.task_runner)
        self.running = True
        self.worker_thread.start()

    def stop(self):
        """
        Stop the work thread
        """
        self.running = False
        self.add_task(TaskDummy(0))
        self.worker_thread.join()
        

class DBMonitor():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self, table_name, t1_mac, t1_loopback, vni, task_runner, cached_stats):
        self.working = False
        self.appl_db = daemon_base.db_connect("APPL_DB")
        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, table_name)
        self.sel.addSelectable(self.tbl)
        self.t1_mac = t1_mac
        self.t1_loopback = t1_loopback
        self.vni = vni
        self.task_runner = task_runner
        self.cached_stats = cached_stats

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
        af = ip_interface(card_vip).version
        if op == 'SET':
            interval = int(fvp.get("interval", DEFAULT_INTERVAL))
            multiplier = int(fvp.get("multiplier", DEFAULT_MULTIPLIER))
            filter_mac = fvp.get("overlay_dmac")
            logger_helper.log_notice("Got new vnet ping task T0 Loopback: {} VIP: {}".format(t0_loopback, card_vip))
            self.cached_stats.add_entry(t0_loopback, card_vip)
            ping_task = TaskPing(
                                expected_running_timestamp_ms=monotonic_ms() + random.randint(0, interval), # A random delay is added to avoid burst request
                                t1_mac=self.t1_mac,
                                t1_loopback=self.t1_loopback[af],
                                t0_loopback=t0_loopback,
                                special_mac=filter_mac,
                                card_vip=card_vip,
                                vni=self.vni,
                                interval_ms=interval,
                                multiplier=multiplier,
                                seq_num=0)
            self.task_runner.add_task(ping_task)
        else:
            logger_helper.log_notice("Removed new vnet ping task T0 Loopback: {} VIP: {}".format(t0_loopback, card_vip))
            self.cached_stats.remove_entry(t0_loopback, card_vip)

    def start(self):
        """
        Start monitoring the table
        """
        self.working = True
        SELECT_TIMEOUT_MSECS = 1000
        while True:
            (state, c) = self.sel.select(SELECT_TIMEOUT_MSECS)
            if not self.working:
                break
            if state == swsscommon.Select.OBJECT:
                (key, op, fvp) = self.tbl.pop()
                self.process_new_entry(key, op, dict(fvp))
    
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
        seq_number = int.from_bytes(load[4:8], 'big')
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
        netlink_api = IPRoute()
        up_ifaces = []
        for iface in ifaces:
            try:
                status = netlink_api.link("get", ifname=iface)
            except NetlinkError:
                continue
        
            if status[0]['state'] == 'up':
                up_ifaces.append(iface)
        
        return up_ifaces


    def _wait_intf_up_msg(self):
        """
        Wait for RTM_NEWLINK message from IPRoute. 
        """
        msgs = []
        with IPRoute() as ipr:
            ipr.bind()
            for msg in ipr.get():
                if msg['event'] == "RTM_NEWLINK" and msg.get('state', '') == 'up':
                    msgs.append(msg)
        
        return msgs


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
        while True:
            msgs = self._wait_intf_up_msg()
            if self.sniffer == None or not self.working:
                # Stop working
                break
            # At least one interface is up now, need to restart the sniffer to add the interface into iface list
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
        self.sniffer = AsyncSniffer(iface=self.sniffing_iface, filter=self.sniffstring, prn=process_packet, store=False)
        self.sniffer.start()
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

