import ipaddress
import syslog
from abc import abstractmethod
from swsscommon import swsscommon

DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
DHCP_SERVER_IPV4_PORT = "DHCP_SERVER_IPV4_PORT"
DHCP_SERVER_IPV4_RANGE = "DHCP_SERVER_IPV4_RANGE"
VLAN = "VLAN"
VLAN_MEMBER = "VLAN_MEMBER"
VLAN_INTERFACE = "VLAN_INTERFACE"
DEFAULT_SELECT_TIMEOUT = 5000  # millisecond


class DhcpDbMonitor(object):
    def __init__(self, db_connector, select_timeout=DEFAULT_SELECT_TIMEOUT):
        self.db_connector = db_connector
        self.sel = swsscommon.Select()
        self.select_timeout = select_timeout

    @abstractmethod
    def subscribe_table(self):
        """
        Subcribe db table to monitor
        """
        raise NotImplementedError

    @abstractmethod
    def _do_check(self):
        """
        Check whether interested table content changed
        """
        raise NotImplementedError

    def check_db_update(self, check_param):
        """
        Fetch db and check update
        """
        state, _ = self.sel.select(self.select_timeout)
        if state == swsscommon.Select.TIMEOUT or state != swsscommon.Select.OBJECT:
            return False
        return self._do_check(check_param)


class DhcpRelaydDbMonitor(DhcpDbMonitor):
    subscribe_dhcp_server_table = None
    subscribe_vlan_table = None
    subscribe_vlan_intf_table = None

    def subscribe_table(self):
        self.subscribe_dhcp_server_table = swsscommon.SubscriberStateTable(self.db_connector.config_db,
                                                                           DHCP_SERVER_IPV4)
        self.subscribe_vlan_table = swsscommon.SubscriberStateTable(self.db_connector.config_db, VLAN)
        self.subscribe_vlan_intf_table = swsscommon.SubscriberStateTable(self.db_connector.config_db, VLAN_INTERFACE)
        # Subscribe dhcp_server_ipv4 and vlan/vlan_interface table. No need to subscribe vlan_member table
        self.sel.addSelectable(self.subscribe_dhcp_server_table)
        self.sel.addSelectable(self.subscribe_vlan_table)
        self.sel.addSelectable(self.subscribe_vlan_intf_table)

    def _do_check(self, check_param):
        if "enabled_dhcp_interfaces" not in check_param:
            syslog.syslog(syslog.LOG_ERR, "Cannot get enabled_dhcp_interfaces")
            return (True, True, True)
        enabled_dhcp_interfaces = check_param["enabled_dhcp_interfaces"]
        return (self._check_dhcp_server_update(enabled_dhcp_interfaces),
                self._check_vlan_update(enabled_dhcp_interfaces),
                self._check_vlan_intf_update(enabled_dhcp_interfaces))

    def _check_dhcp_server_update(self, enabled_dhcp_interfaces):
        """
        Check dhcp_server_ipv4 table
        Args:
            enabled_dhcp_interfaces: DHCP interface that enabled dhcp_server
        Returns:
            Whether need to refresh
        """
        need_refresh = False
        while self.subscribe_dhcp_server_table.hasData():
            key, op, entry = self.subscribe_dhcp_server_table.pop()
            if op == "SET":
                for field, value in entry:
                    if field != "state":
                        continue
                    # Only if new state is not consistent with old state, we need to refresh
                    if key in enabled_dhcp_interfaces and value == "disabled":
                        need_refresh = True
                    elif key not in enabled_dhcp_interfaces and value == "enabled":
                        need_refresh = True
            # For del operation, we can skip disabled change
            if op == "DEL":
                if key in enabled_dhcp_interfaces:
                    need_refresh = True
        return need_refresh

    def _check_vlan_update(self, enabled_dhcp_interfaces):
        """
        Check vlan table
        Args:
            enabled_dhcp_interfaces: DHCP interface that enabled dhcp_server
        Returns:
            Whether need to refresh
        """
        need_refresh = False
        while self.subscribe_vlan_table.hasData():
            key, op, _ = self.subscribe_vlan_table.pop()
            # For vlan doesn't have related dhcp entry, not need to refresh dhcrelay process
            if key not in enabled_dhcp_interfaces:
                continue
            need_refresh = True
        return need_refresh

    def _check_vlan_intf_update(self, enabled_dhcp_interfaces):
        """
        Check vlan_interface table
        Args:
            enabled_dhcp_interfaces: DHCP interface that enabled dhcp_server
        Returns:
            Whether need to refresh
        """
        need_refresh = False
        while self.subscribe_vlan_intf_table.hasData():
            key, _, _ = self.subscribe_vlan_intf_table.pop()
            splits = key.split("|")
            vlan_name = splits[0]
            ip_address = splits[1].split("/")[0] if len(splits) > 1 else None
            if vlan_name not in enabled_dhcp_interfaces:
                continue
            if ip_address is None or ipaddress.ip_address(ip_address).version != 4:
                continue
            need_refresh = True
        return need_refresh
