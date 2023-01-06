# configDB helper functions

from swsscommon import swsscommon
import netaddr
from sonic_py_common import logger
from swsscommon.swsscommon import ConfigDBConnector
from pyroute2 import IPRoute
from ipaddress import ip_interface

#
# Config
#

DEFAULT_TCP_PORT = 1021
DEFAULT_VXLAN_PORT = 65330
DEFAULT_TCP_OPTIONS = 253
DEFAULT_TTL = 2
REDIS_TIMEOUT_MS = 0
CFG_VLAN_TABLE_NAME = 'VLAN'
CFG_VLAN_VLANID_KEY_NAME = 'vlanid'
CFG_VLAN_MONVLAN_KEY_NAME = 'host_ifname'
CFG_METADATA_LOCALHOST_ENTRY_NAME = 'localhost'
CFG_METADATA_MAC_KEY_NAME = 'mac'
PORTCHANNEL_INTERFACE_TABLE = 'PORTCHANNEL_INTERFACE'
RTM_NEWLINK = 'RTM_NEWLINK'

#
# Global logger instance
#
vnetlogger = logger.Logger()


###

#
# Get an ipv4 address from a given string.
# Return None if the given string is not a valid ipv4 address.
#

def get_ipv4_address(value):
    if not value:
        return None

    try:
        addr = netaddr.IPNetwork(str(value))
    except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
        return None

    if addr.version == 4:
        return str(addr.ip)
    else:
        return None

#
# Get a value of a key from ConfigDB's tablename|entry_name
#

def get_configdb_info(table_name, entry_name, key_name):
    ret_value = None
    if entry_name is None or key_name is None:
        return ret_value

    try:
        config_db = swsscommon.DBConnector("CONFIG_DB", REDIS_TIMEOUT_MS, True)
        vlan_table = swsscommon.Table(config_db, table_name)
        (status, tuples) = vlan_table.get(entry_name)
        entry_info = dict(tuples)
        ret_value = entry_info[key_name]
    except KeyError as ke:
        vnetlogger.log_error("KeyError {0}".format(str(ke)))

    return ret_value

#
# Extract Loopback0 ipv4 address
#

def get_loopback_ipv4():
    loopback0_ipv4 = None

    try:
        config_db = swsscommon.DBConnector("CONFIG_DB", REDIS_TIMEOUT_MS, True)
        loopback_interface = swsscommon.Table(config_db, swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME)
        for loopback in loopback_interface.getKeys():
            if loopback.startswith("Loopback0|"):
                loopback0_prefix_str = loopback.replace("Loopback0|", "")
                ipv4_address = get_ipv4_address(loopback0_prefix_str)
                if ipv4_address is not None:
                    loopback0_ipv4 = ipv4_address
                    break
    except ValueError as ve:
        vnetlogger.log_error("ValueError {0}".format(str(ve)))

    return loopback0_ipv4


#
# Get the vlanid from Vlan interface name
#

def get_mon_vlan_id(vlan_interface_name):
    return get_configdb_info(CFG_VLAN_TABLE_NAME, vlan_interface_name, CFG_VLAN_VLANID_KEY_NAME)


#
# Get the host ifname from Vlan interface name
#

def get_mon_vlan_host_ifname(vlan_interface_name):
    return get_configdb_info(CFG_VLAN_TABLE_NAME, vlan_interface_name, CFG_VLAN_MONVLAN_KEY_NAME)


#
# Get the dut mac address from DEVICE_METADATA|localhost
#

def get_localhost_mac():
    return get_configdb_info(swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, CFG_METADATA_LOCALHOST_ENTRY_NAME, CFG_METADATA_MAC_KEY_NAME)


#
# Get active portchannels and monitor portchannel events
#
class PortChannelHandler(object):

    def __init__(self):
        self.config_db = ConfigDBConnector()
        self.config_db.connect()
        self._portchannel_intfs = None
        self.netlink_api = IPRoute()

    #
    # Gets all portchannel interfaces and IPv4 addresses in config DB
    # Returns:
    #     (list) Tuples of a portchannel interface name (str) and
    #            associated IPv4 address (str)
    #
    @property
    def portchannel_intfs(self):
        if self._portchannel_intfs is None:
            intf_keys = self.config_db.get_keys(PORTCHANNEL_INTERFACE_TABLE)
            portchannel_intfs = []

            for key in intf_keys:
                if isinstance(key, tuple) and len(key) > 1:
                    if ip_interface(key[1]).version == 4:
                        portchannel_intfs.append(key)

            self._portchannel_intfs = portchannel_intfs

        return self._portchannel_intfs

    #
    # Gets a mapping of interface kernel indices to portchannel interfaces
    # Returns:
    #     (list) integers representing kernel indices
    #
    def get_portchannel_index_mapping(self):
        index_map = {}
        for portchannel in self.portchannel_intfs:
            index = self.netlink_api.link_lookup(ifname=portchannel[0])[0]
            index_map[index] = portchannel

        return index_map

    #
    # Retrieves the portchannels which are operationally up
    # Returns:
    #     (list) of interface names which are up, as strings
    #
    def get_up_portchannels(self):
        pc_index_map = self.get_portchannel_index_mapping()
        pc_indices = list(pc_index_map.keys())
        link_statuses = self.netlink_api.get_links(*pc_indices)
        up_portchannels = []

        for status in link_statuses:
            if status['state'] == 'up':
                port_index = status['index']
                up_portchannels.append(str(pc_index_map[port_index][0]))

        return up_portchannels

    #
    # Gathers RTM_NEWLINK messages
    # Returns:
    #    (list) containing any received messages
    #
    def wait_for_netlink_msgs(self):
        msgs = []
        with IPRoute() as ipr:
            ipr.bind()
            for msg in ipr.get():
                if msg['event'] == RTM_NEWLINK:
                    msgs.append(msg)

        return msgs

    #
    # Determines if the packet sniffer needs to be restarted
    #
    def sniffer_restart_required(self, messages):
        # A restart is required if all of the following conditions are met:
        #     1. A netlink message of type RTM_NEWLINK is received
        #        (this is checked by `wait_for_netlink_msgs`)
        #     2. The interface index of the message corresponds to a portchannel interface
        #     3. The state of the interface in the message is 'up'
        #
        # We do not care about an interface going down since the sniffer is able
        # to continue sniffing on the other interfaces. However, if an interface has
        # gone down and come back up, we need to restart the sniffer to be able to sniff 
        # traffic on the interface that has come back up.
        #
        pc_index_map = self.get_portchannel_index_mapping()
        for msg in messages:
            if msg['index'] in pc_index_map:
                if msg['state'] == 'up':
                    vnetlogger.log_info('vnetping: {0} came back up, sniffer restart required'.format(str(pc_index_map[msg['index']])))
                    return True
        return False

