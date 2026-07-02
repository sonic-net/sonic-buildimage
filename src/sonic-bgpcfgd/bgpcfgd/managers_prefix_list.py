from .manager import Manager
from .log import log_debug, log_warn, log_info
from swsscommon import swsscommon
import netaddr

PREFIX_TYPE_CONFIG = {
    "ANCHOR_PREFIX": {
        "add_template": "bgpd/radian/add_radian",
        "del_template": "bgpd/radian/del_radian",
        "allowed_devices": [
            ("SpineRouter", "UpstreamLC"),
            ("UpperSpineRouter", None),
        ],
        "prefix_list_name": lambda ipv: "ANCHOR_CONTRIBUTING_ROUTES",
        "log_label": "Anchor prefix",
        "log_label_target": "radian",
    },
    "SUPPRESS_PREFIX": {
        "add_template": "bgpd/suppress_prefix/add_suppress_prefix",
        "del_template": "bgpd/suppress_prefix/del_suppress_prefix",
        "allowed_devices": None,
        "prefix_list_name": lambda ipv: "SUPPRESS_IPV4_PREFIX" if ipv == "ip" else "SUPPRESS_IPV6_PREFIX",
        "log_label": "Suppress prefix",
        "log_label_target": "suppress_prefix",
    },
}

class PrefixListMgr(Manager):
    """This class responds to changes in the PREFIX_LIST table"""

    def __init__(self, common_objs, db, table):
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        self.templates = {}
        for cfg in PREFIX_TYPE_CONFIG.values():
            self.templates[cfg["add_template"]] = common_objs['tf'].from_file(cfg["add_template"] + ".conf.j2")
            self.templates[cfg["del_template"]] = common_objs['tf'].from_file(cfg["del_template"] + ".conf.j2")
        super(PrefixListMgr, self).__init__(
            common_objs,
            [
                ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/type"),
                ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),
            ],
            db,
            table,
        )

    def _is_device_allowed(self, device_type, device_subtype, allowed_list):
        if allowed_list is None:
            return True
        for allowed_type, allowed_subtype in allowed_list:
            if device_type == allowed_type:
                if allowed_subtype is None or device_subtype == allowed_subtype:
                    return True
        return False

    def generate_prefix_list_config(self, prefix_type, data, add):
        type_cfg = PREFIX_TYPE_CONFIG.get(prefix_type)
        if type_cfg is None:
            log_warn("PrefixListMgr:: Prefix type '%s' is not supported" % prefix_type)
            return False

        if not self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost"):
            log_info("PrefixListMgr:: Device metadata is not ready yet")
            return False
        metadata = self.directory.get_path("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost")
        # bgp_asn is required for all prefix types: ANCHOR_PREFIX templates use it
        # directly (router bgp <asn>), and while SUPPRESS_PREFIX templates don't
        # reference it today, prefix-list operations are inherently BGP features —
        # any device managing prefix lists will be running BGP with an ASN configured.
        # Requiring bgp_asn upfront keeps templates free to use it when expanded.
        try:
            device_type = metadata["type"]
            device_subtype = metadata.get("subtype", "")
            bgp_asn = metadata["bgp_asn"]
        except KeyError as e:
            log_warn("PrefixListMgr:: Missing metadata key: %s" % e)
            return False

        if not self._is_device_allowed(device_type, device_subtype, type_cfg["allowed_devices"]):
            device_desc = "%s/%s" % (device_type, device_subtype) if device_subtype else device_type
            log_warn("PrefixListMgr:: Device type %s not supported for %s" % (device_desc, prefix_type))
            return False

        data["bgp_asn"] = bgp_asn
        data["device_type"] = device_type
        data["device_subtype"] = device_subtype
        pl_overrides = self.constants.get("bgp", {}).get("prefix_list", {}).get(prefix_type, {})
        name_key = "ipv4_name" if data["ipv"] == "ip" else "ipv6_name"
        data["prefix_list_name"] = pl_overrides.get(name_key, type_cfg["prefix_list_name"](data["ipv"]))

        template_key = type_cfg["add_template"] if add else type_cfg["del_template"]
        cmd = "\n" + self.templates[template_key].render(data=data)
        self.cfg_mgr.push(cmd)

        action = "added to" if add else "removed from"
        log_debug("PrefixListMgr:: %s %s %s %s configuration" % (type_cfg["log_label"], data["prefix"], action, type_cfg["log_label_target"]))
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
            data = {}
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