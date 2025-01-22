import ipaddress
import sys
import syslog
from abc import abstractmethod
from swsscommon import swsscommon

DEFAULT_SELECT_TIMEOUT = 5000  # millisecond
DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
DHCP_SERVER_IPV4_PORT = "DHCP_SERVER_IPV4_PORT"
DHCP_SERVER_IPV4_RANGE = "DHCP_SERVER_IPV4_RANGE"
DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS"
VLAN = "VLAN"
VLAN_MEMBER = "VLAN_MEMBER"
VLAN_INTERFACE = "VLAN_INTERFACE"
FEATURE = "FEATURE"
MID_PLANE_BRIDGE = "MID_PLANE_BRIDGE"
DPUS = "DPUS"


class ConfigDbEventChecker(object):
    table_name = ""
    subscriber_state_table = None
    enabled = False

    def __init__(self, sel, db):
        """
        Init function
        Args:
            sel: select object to manage subscribe table
            db_connector: db connector
        """
        self.sel = sel
        self.db = db
        self.subscriber_state_table = None
        self.enabled = False

    @classmethod
    def get_parameter_by_name(cls, db_snapshot, param_name):
        """
        Check whether db_snapshot valid
        Args:
            db_snapshot: dict contains db_snapshot param
            param_name: parameter name need to check
        Returns:
            If param_name in db_snapshot return tuple of (True, parameter), else return tuple of (False, None)
        """
        if param_name not in db_snapshot:
            return False, None
        return True, db_snapshot[param_name]

    def is_enabled(self):
        """
        Check whether checker is enabled
        Returns:
            If enabled, return True. Else return False
        """
        return self.enabled

    def enable(self):
        """
        Enable checker by subscribe table
        Args:
            db: db object
        """
        if self.enabled:
            syslog.syslog(syslog.LOG_ERR, "Cannot enable {} checker due to it is enabled"
                          .format(self.table_name))
            sys.exit(1)
        self.subscriber_state_table = swsscommon.SubscriberStateTable(self.db, self.table_name)
        self.sel.addSelectable(self.subscriber_state_table)
        self.enabled = True

    def disable(self):
        """
        Disable checker
        """
        if not self.enabled:
            syslog.syslog(syslog.LOG_ERR, "Cannot disable {} checker due to it is disabled"
                          .format(self.table_name))
            sys.exit(1)
        self.sel.removeSelectable(self.subscriber_state_table)
        self.enabled = False

    def clear_event(self):
        """
        Clear update event of subscirbe table
        """
        if not self.enabled:
            syslog.syslog(syslog.LOG_ERR, "Cannot clear event for table {} due to it is disabled"
                          .format(self.table_name))
            sys.exit(1)
        while self.subscriber_state_table.hasData():
            _, _, _ = self.subscriber_state_table.pop()

    @abstractmethod
    def _get_parameter(self, db_snapshot):
        """
        Get paramter depends on subclass
        Args:
            db_snapshot: dict of db snapshot
        """
        raise NotImplementedError

    @abstractmethod
    def _process_check(self, key, op, entry, parameter):
        """
        Check whether this event contains value we interested
        Args:
            key: key of event
            op: operation of event
            entry: operation entry of event
            paramter: parameter used in check
        Returns:
            If contains, return True, else return False
        """
        raise NotImplementedError

    def check_update_event(self, db_snapshot):
        """
        Function to check whether interested field changed in subscribe table
        Args:
            db_snapshot: dict contains db_snapshot param
        Returns:
            If changed, return True, else return False
        """
        res, parameter = self._get_parameter(db_snapshot)
        if not res:
            return True
        need_refresh = False
        while self.subscriber_state_table.hasData():
            key, op, entry = self.subscriber_state_table.pop()
            need_refresh |= self._process_check(key, op, entry, parameter)
            if need_refresh:
                self.clear_event()
                return True
        return False

    def _check_db_snapshot(self, db_snapshot, param_name):
        """
        Check whether db_snapshot valid
        Args:
            db_snapshot: dict contains db_snapshot param
            param_name: parameter name need to check
        Returns:
            If param_name in db_snapshot return True, else return False
        """
        if param_name not in db_snapshot:
            syslog.syslog(syslog.LOG_ERR, "Expected param: {} is not in db_snapshot".format(param_name))
            return False

        return True

    def get_class_name(self):
        """
        Get class name of this object
        """
        return type(self).__name__


class DhcpServerTableCfgChangeEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in all DHCP server related config change event in DHCP_SERVER_IPV4 table
    """
    table_name = DHCP_SERVER_IPV4

    def __init__(self, sel, db):
        self.table_name = DHCP_SERVER_IPV4
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        # If old state is enabled, need refresh
        if key in enabled_dhcp_interfaces:
            return True
        elif op == "SET":
            for field, value in entry:
                if field != "state":
                    continue
                # If old state is not consistent with new state, need refresh
                if value == "enabled":
                    return True
        return False


class DhcpServerTableIntfEnablementEventChecker(ConfigDbEventChecker):
    """
    This event checker only interested in DHCP interface enabled/disabled in DHCP_SERVER_IPV4 table
    """
    table_name = DHCP_SERVER_IPV4

    def __init__(self, sel, db):
        self.table_name = DHCP_SERVER_IPV4
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        if op == "SET":
            for field, value in entry:
                if field != "state":
                    continue
                # Only if new state is not consistent with old state, we need to refresh
                if key in enabled_dhcp_interfaces and value == "disabled":
                    return True
                elif key not in enabled_dhcp_interfaces and value == "enabled":
                    return True
        # For del operation, we can skip disabled change
        if op == "DEL":
            if key in enabled_dhcp_interfaces:
                return True
        return False


class DhcpPortTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in DHCP_SERVER_IPV4_PORT table
    """
    table_name = DHCP_SERVER_IPV4_PORT

    def __init__(self, sel, db):
        self.table_name = DHCP_SERVER_IPV4_PORT
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        dhcp_interface = key.split("|")[0]
        # If dhcp interface is enabled, need to generate new configuration
        if dhcp_interface in enabled_dhcp_interfaces:
            self.clear_event()
            return True
        return False


class DhcpRangeTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in DHCP_SERVER_IPV4_RANGE table
    """
    table_name = DHCP_SERVER_IPV4_RANGE

    def __init__(self, sel, db):
        self.table_name = DHCP_SERVER_IPV4_RANGE
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "used_range")

    def _process_check(self, key, op, entry, used_range):
        # If range is used, need to generate new configuration
        if key in used_range:
            self.clear_event()
            return True
        return False


class DhcpOptionTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS table
    """
    table_name = DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS

    def __init__(self, sel, db):
        self.table_name = DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "used_options")

    def _process_check(self, key, op, entry, used_options):
        # If option is used, need to generate new configuration
        if key in used_options:
            self.clear_event()
            return True
        return False


class VlanTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in VLAN table
    """
    table_name = VLAN

    def __init__(self, sel, db):
        self.table_name = VLAN
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        # For vlan doesn't have related dhcp entry, not need to refresh dhcrelay process
        if key in enabled_dhcp_interfaces:
            self.clear_event()
            return True
        return False


class VlanIntfTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in VLAN_INTERFACE table
    """
    table_name = VLAN_INTERFACE

    def __init__(self, sel, db):
        self.table_name = VLAN_INTERFACE
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        splits = key.split("|")
        vlan_name = splits[0]
        ip_address = splits[1].split("/")[0] if len(splits) > 1 else None
        # For vlan doesn't have related dhcp entry, not need to refresh dhcrelay process
        if vlan_name in enabled_dhcp_interfaces and ip_address is not None and \
           ipaddress.ip_address(ip_address).version == 4:
            self.clear_event()
            return True
        return False


class VlanMemberTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in VLAN_MEMBER table
    """
    table_name = VLAN_MEMBER

    def __init__(self, sel, db):
        self.table_name = VLAN_MEMBER
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        dhcp_interface = key.split("|")[0]
        # If dhcp interface is enabled, need to generate new configuration
        if dhcp_interface in enabled_dhcp_interfaces:
            self.clear_event()
            return True
        return False


class MidPlaneTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in MID_PLANE_BRIDGE table
    """
    table_name = MID_PLANE_BRIDGE

    def __init__(self, sel, db):
        self.table_name = MID_PLANE_BRIDGE
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "enabled_dhcp_interfaces")

    def _process_check(self, key, op, entry, enabled_dhcp_interfaces):
        if op == "DEL":
            return True
        for field, value in entry:
            if field == "bridge" and value in enabled_dhcp_interfaces:
                return True
        return False


class DpusTableEventChecker(ConfigDbEventChecker):
    """
    This event checker interested in changes in DPUS table
    """
    table_name = DPUS

    def __init__(self, sel, db):
        self.table_name = DPUS
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return True, None

    def _process_check(self, key, op, entry, param):
        return True


class DhcpServerFeatureStateChecker(ConfigDbEventChecker):
    """
    This event checker interested in dhcp_server feature state change in FEATURE table
    """
    table_name = FEATURE

    def __init__(self, sel, db):
        self.table_name = FEATURE
        ConfigDbEventChecker.__init__(self, sel, db)

    def _get_parameter(self, db_snapshot):
        return ConfigDbEventChecker.get_parameter_by_name(db_snapshot, "dhcp_server_feature_enabled")

    def _process_check(self, key, op, entry, dhcp_server_feature_enabled):
        if key != "dhcp_server":
            return False
        if op == "DEL":
            return dhcp_server_feature_enabled
        for field, value in entry:
            if field != "state":
                continue
            return value == "enabled" and not dhcp_server_feature_enabled or \
                value == "disabled" and dhcp_server_feature_enabled
        return False


def _enable_monitor_checkers(checker_names, checker_dict):
    """
    Enable checkers
    Args:
        checker_names: set of tables checker to be enable
        checker_dict: check_dict in monitor
    """
    for checker in checker_names:
        if checker not in checker_dict:
            syslog.syslog(syslog.LOG_ERR, "Cannot find checker for {} in checker_dict".format(checker))
            continue
        checker_dict[checker].enable()


def _disable_monitor_checkers(checker_names, checker_dict):
    """
    Disable checkers
    Args:
        checker_names: set contains name of tables need to be disable
        checker_dict: check_dict in monitor
    """
    for checker in checker_names:
        if checker not in checker_dict:
            syslog.syslog(syslog.LOG_ERR, "Cannot find checker for {} in checker_dict".format(checker))
            continue
        checker_dict[checker].disable()


class DhcpRelaydDbMonitor(object):
    checker_dict = {}

    def __init__(self, db_connector, sel, checkers, select_timeout=DEFAULT_SELECT_TIMEOUT):
        self.db_connector = db_connector
        self.sel = sel
        self.select_timeout = select_timeout
        self.checker_dict = {}
        for checker in checkers:
            self.checker_dict[checker.get_class_name()] = checker

    def enable_checkers(self, checker_names):
        """
        Enable checkers
        Args:
            checker_names: set of tables checker to be enable
        """
        _enable_monitor_checkers(checker_names, self.checker_dict)

    def disable_checkers(self, checker_names):
        """
        Disable checkers
        Args:
            checker_names: set contains name of tables need to be disable
        """
        _disable_monitor_checkers(checker_names, self.checker_dict)

    def check_db_update(self, db_snapshot):
        """
        Fetch db and check update
        Args:
            db_snapshot: dict contains db snapshot parameter
        Returns:
            Tuple of dhcp_server table result, vlan table result, vlan_intf table result
        """
        state, _ = self.sel.select(self.select_timeout)
        if state == swsscommon.Select.TIMEOUT or state != swsscommon.Select.OBJECT:
            return {}
        check_res = {}
        for name, checker in self.checker_dict.items():
            if not checker.is_enabled():
                continue
            check_res[name] = checker.check_update_event(db_snapshot)
        return check_res


class DhcpServdDbMonitor(object):
    checker_dict = {}

    def __init__(self, db_connector, sel, checkers, select_timeout=DEFAULT_SELECT_TIMEOUT):
        self.db_connector = db_connector
        self.sel = sel
        self.select_timeout = select_timeout
        self.checker_dict = {}
        for checker in checkers:
            self.checker_dict[checker.get_class_name()] = checker

    def disable_checkers(self, checker_names):
        """
        Disable checkers
        Args:
            checker_names: set contains name of tables need to be disable
        """
        _disable_monitor_checkers(checker_names, self.checker_dict)

    def enable_checkers(self, checker_names):
        """
        Enable checkers
        Args:
            checker_names: set contains name of tables need to be enable
        """
        _enable_monitor_checkers(checker_names, self.checker_dict)

    def check_db_update(self, db_snapshot):
        """
        Fetch db and check update
        Args:
            db_snapshot: dict contains db snapshot parameter
        Returns:
            Whether need to refresh config file for kea-dhcp-server
        """
        state, _ = self.sel.select(self.select_timeout)
        if state == swsscommon.Select.TIMEOUT or state != swsscommon.Select.OBJECT:
            return False
        need_refresh = False
        for checker in self.checker_dict.values():
            if not checker.is_enabled():
                continue
            if need_refresh:
                checker.clear_event()
            else:
                need_refresh |= checker.check_update_event(db_snapshot)
        return need_refresh
