import re

import netaddr

from .manager import Manager
from .log import log_debug, log_warn, log_info

ADD_TEMPLATE = "bgpd/prefix_list/add_prefix_list"
DEL_TEMPLATE = "bgpd/prefix_list/del_prefix_list"
PREFIX_LIST_NAME_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.:-]*$")


class PrefixListMgr(Manager):
    """This class responds to changes in the PREFIX_LIST table"""

    def __init__(self, common_objs, db, table):
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.templates = {
            ADD_TEMPLATE: common_objs['tf'].from_file(ADD_TEMPLATE + ".conf.j2"),
            DEL_TEMPLATE: common_objs['tf'].from_file(DEL_TEMPLATE + ".conf.j2"),
        }
        super(PrefixListMgr, self).__init__(common_objs, [], db, table)

    def generate_prefix_list_config(self, prefix_type, data, add):
        if not PREFIX_LIST_NAME_PATTERN.fullmatch(prefix_type):
            log_warn("PrefixListMgr:: Prefix list name '%s' is invalid" % prefix_type)
            return False

        address_family = "V4" if data["ipv"] == "ip" else "V6"
        data["prefix_list_name"] = "%s_%s" % (prefix_type, address_family)

        if not add and data.pop("delete_by_name", False):
            cmd = "\nno %s prefix-list %s" % (data["ipv"], data["prefix_list_name"])
            self.cfg_mgr.push(cmd)
            log_warn(
                "PrefixListMgr:: Missing cached fields for delete of prefix list '%s'; "
                "issued best-effort delete-by-name" % data["prefix_list_name"]
            )
            return True

        data["action"] = data.get("action") or "permit"

        template_key = ADD_TEMPLATE if add else DEL_TEMPLATE
        cmd = "\n" + self.templates[template_key].render(data=data)
        self.cfg_mgr.push(cmd)

        action = "added to" if add else "removed from"
        log_debug("PrefixListMgr:: Prefix %s %s %s configuration" % (data["prefix"], action, data["prefix_list_name"]))
        return True

    def set_handler(self, key, data):
        log_debug("PrefixListMgr:: set handler")
        if '|' in key:
            prefix_type, prefix_str = key.split('|', 1)
            try:
                prefix = netaddr.IPNetwork(str(prefix_str))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_type))
                return True
            data["prefix"] = str(prefix.cidr)
            data["prefixlen"] = prefix.prefixlen
            data["ipv"] = self.get_ip_type(prefix)
            if self.generate_prefix_list_config(prefix_type, data, add=True):
                log_info("PrefixListMgr:: %s %s configuration generated" % (prefix_type, data["prefix"]))
                self.directory.put(self.db_name, self.table_name, key, data)
                log_info("PrefixListMgr:: set %s" % key)
        return True

    def del_handler(self, key):
        log_debug("PrefixListMgr:: del handler")
        if '|' in key:
            prefix_type, prefix_str = key.split('|', 1)
            try:
                prefix = netaddr.IPNetwork(str(prefix_str))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_type))
                return True
            table_data = self.directory.get_slot(self.db_name, self.table_name)
            data = dict(table_data.get(key, {}))
            data["delete_by_name"] = not bool(data)
            data["prefix"] = str(prefix.cidr)
            data["prefixlen"] = prefix.prefixlen
            data["ipv"] = self.get_ip_type(prefix)
            if self.generate_prefix_list_config(prefix_type, data, add=False):
                log_info("PrefixListMgr:: %s %s configuration deleted" % (prefix_type, data["prefix"]))
                self.directory.remove(self.db_name, self.table_name, key)
                log_info("PrefixListMgr:: deleted %s" % key)
        return True

    def get_ip_type(self, prefix: netaddr.IPNetwork):
        if prefix.version == 4:
            return "ip"
        elif prefix.version == 6:
            return "ipv6"
        else:
            return None