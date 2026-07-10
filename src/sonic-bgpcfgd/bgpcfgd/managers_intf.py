import netaddr

from .log import log_warn
from .manager import Manager


class InterfaceMgr(Manager):
    """ This class updates the Directory object when interface-related table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(InterfaceMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        """ Implementation of 'SET' command.
        Similar to BGPDataBaseMgr but enriches data object with additional data """
        # Interface table can have two keys,
        # one with ip prefix and one without ip prefix
        if '|' in key:
            interface_name, network_str = key.split('|', 1)
            try:
                network = netaddr.IPNetwork(str(network_str))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("Subnet '%s' format is wrong for interface '%s'" % (network_str, interface_name))
                return True
            data["interface"] = interface_name
            data["prefixlen"] = str(network.prefixlen)
            ip = str(network.ip)
            self.directory.put("LOCAL", "local_addresses", ip, data)
        self.directory.put(self.db_name, self.table_name, key, data)
        self.directory.put("LOCAL", "interfaces", key, data)
        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command
        Also removes data object enrichment """
        if '|' in key:
            interface, network = key.split('|', 1)
            try:
                network = netaddr.IPNetwork(str(network))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("Subnet '%s' format is wrong for interface '%s'" % (network, interface))
                return
            ip = str(network.ip)
            self.directory.remove("LOCAL", "local_addresses", ip)
        self.directory.remove(self.db_name, self.table_name, key)
        self.directory.remove("LOCAL", "interfaces", key)