from .manager import Manager
from .log import log_debug, log_warn, log_info
from swsscommon import swsscommon
import netaddr

class PrefixListMgr(Manager):
    """This class responds to changes in the PREFIX_LIST table"""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        self.templates = {
            "add_radian": common_objs['tf'].from_file("bgpd/radian/add_radian.conf.j2"),
            "del_radian": common_objs['tf'].from_file("bgpd/radian/del_radian.conf.j2"),
        }
        super(PrefixListMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def generate_prefix_list_config(self, data, add):
        """
        Generate the prefix list configuration from the template
        :param data: data from the PREFIX_LIST table
        :return: rendered configuration
        """
        cmd = "\n"
        metadata = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]
        try:
            bgp_asn = metadata["bgp_asn"]
            localhost_type = metadata["type"]
            subtype = metadata["subtype"]
        except KeyError as e:
            log_warn(f"PrefixListMgr:: Missing metadata key: {e}")
            return False

        if data["prefix_list_name"] != "ANCHOR_PREFIX":
            log_warn("PrefixListMgr:: Prefix list %s is not supported" % data["prefix_list_name"])
            return False
        if localhost_type not in ["UpperSpineRouter", "SpineRouter"] or (localhost_type == "SpineRouter" and subtype != "UpstreamLC"):
            log_warn("PrefixListMgr:: Prefix list %s is only supported on Upstream SpineRouter" % data["prefix_list_name"])
            return False

        # Add the anchor prefix to the radian configuration
        data["bgp_asn"] = bgp_asn
        if add:
            # add some way of getting this asn list from the database in the future
            cmd += self.templates["add_radian"].render(data=data)
            log_debug("PrefixListMgr:: Anchor prefix %s added to radian configuration" % data["prefix"])
        else:
            cmd += self.templates["del_radian"].render(data=data)
            log_debug("PrefixListMgr:: Anchor prefix %s removed from radian configuration" % data["prefix"])	
        self.cfg_mgr.push(cmd)
        return True
            
        

    def set_handler(self, key, data):
        log_debug("PrefixListMgr:: set handler")
        if '|' in key:
            prefix_list_name, prefix_str =  key.split('|', 1)
            try:
                prefix = netaddr.IPNetwork(str(prefix_str))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_list_name))
                return True
            data["prefix_list_name"] = prefix_list_name
            data["prefix"] = str(prefix.cidr)
            data["prefixlen"] = prefix.prefixlen
            data["ipv"] = self.get_ip_type(prefix)
            # Generate the prefix list configuration
            if self.generate_prefix_list_config(data, add=True):
                log_info("PrefixListMgr:: %s %s configuration generated" % (prefix_list_name, data["prefix"]))

                self.directory.put(self.db_name, self.table_name, key, data)
                log_info("PrefixListMgr:: set %s" % key)
        return True

    def del_handler(self, key):
        log_debug("PrefixListMgr:: del handler")
        if '|' in key:
            prefix_list_name, prefix_str =  key.split('|', 1)
            try:
                prefix = netaddr.IPNetwork(str(prefix_str))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_list_name))
                return True
            data = {}
            data["prefix_list_name"] = prefix_list_name
            data["prefix"] = str(prefix.cidr)
            data["prefixlen"] = prefix.prefixlen
            data["ipv"] = self.get_ip_type(prefix)
            # remove the prefix list configuration
            if self.generate_prefix_list_config(data, add=False):
                log_info("PrefixListMgr:: %s %s configuration deleted" % (prefix_list_name, data["prefix"]))
                self.directory.remove(self.db_name, self.table_name, key)
                log_info("PrefixListMgr:: deleted %s" % key)
        # Implement deletion logic if necessary
        return True

    def get_ip_type(self, prefix: netaddr.IPNetwork):
        """
        Determine the IP type (IPv4 or IPv6) of a prefix.
        :param prefix: The prefix to check (e.g., "192.168.1.0/24" or "2001:db8::/32")
        :return: "ip" if the prefix is an IPv4 address, "ipv6" if it is an IPv6 address, None if invalid
        """
        if prefix.version == 4:
            return "ip"
        elif prefix.version == 6:
            return "ipv6"
        else:
            return None