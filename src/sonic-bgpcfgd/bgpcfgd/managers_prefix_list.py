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
            ("UpperRegionalHub", None),
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

# A non-empty prefix field selects PREFIX_LIST|<prefix_type>|<entry_name>.
# Otherwise, the prefix is taken from the legacy CIDR key.
PREFIX_FIELD = "prefix"

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

    def _build_dynamic_prefix_list_cmd(self, prefix_type, data, add):
        """Build an entry command, or return None when action is missing."""
        if "action" not in data or not data["action"]:
            return None
        cmd_parts = ["no"] if not add else []
        cmd_parts.extend([data["ipv"], "prefix-list", prefix_type])
        if "seq" in data:
            cmd_parts.extend(["seq", str(data["seq"])])
        cmd_parts.extend([data["action"], data["prefix"]])
        if "ge" in data:
            cmd_parts.extend(["ge", str(data["ge"])])
        if "le" in data:
            cmd_parts.extend(["le", str(data["le"])])
        return "\n" + " ".join(cmd_parts)

    def generate_prefix_list_config(self, prefix_type, data, add, key=None):
        type_cfg = PREFIX_TYPE_CONFIG.get(prefix_type)
        if type_cfg is None:
            missing_action = "action" not in data or not data["action"]
            if not add and missing_action:
                # Best-effort cleanup for delete when cache does not contain full entry fields.
                cmd = "\nno %s prefix-list %s" % (data["ipv"], prefix_type)
                self.cfg_mgr.push(cmd)
                log_warn(
                    "PrefixListMgr:: Missing cached fields for delete of prefix list '%s'; "
                    "issued best-effort delete-by-name" % prefix_type
                )
                log_debug("PrefixListMgr:: Dynamic prefix list %s removed by name fallback" % prefix_type)
                return True

            cmd = self._build_dynamic_prefix_list_cmd(prefix_type, data, add)
            if cmd is None:
                log_warn("PrefixListMgr:: Mandatory field 'action' is not defined for prefix list '%s'" % prefix_type)
                return False
            self.cfg_mgr.push(cmd)
            action = "added to" if add else "removed from"
            log_debug("PrefixListMgr:: Dynamic prefix list %s %s configuration" % (data["prefix"], action))
            return True

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
        if add and key is not None:
            old_data = self._get_cached_entry(key)
            if old_data is not None and old_data["prefix"] != data["prefix"]:
                add_template = self.templates[type_cfg["add_template"]]
                old_cmd = "\n" + add_template.render(data=old_data)
                if old_cmd != cmd:
                    old_config_in_use = any(
                        other_key != key
                        and other_key.split('|', 1)[0] == prefix_type
                        and "\n" + add_template.render(data=other_data) == old_cmd
                        for other_key, other_data in self.directory.get_slot(self.db_name, self.table_name).items()
                    )
                    if old_config_in_use:
                        log_info("PrefixListMgr:: %s changed, previous configuration is still used by another entry" % key)
                    else:
                        # Cached fields retain the original ASN, address family and list name.
                        old_delete_cmd = "\n" + self.templates[type_cfg["del_template"]].render(data=old_data)
                        self.cfg_mgr.push(old_delete_cmd)
                        log_info("PrefixListMgr:: %s changed, removed the previous entry" % key)
        self.cfg_mgr.push(cmd)

        action = "added to" if add else "removed from"
        log_debug("PrefixListMgr:: %s %s %s %s configuration" % (type_cfg["log_label"], data["prefix"], action, type_cfg["log_label_target"]))
        return True

    @staticmethod
    def _parse_prefix(prefix_str):
        """Return a parsed prefix, or None for invalid input."""
        try:
            return netaddr.IPNetwork(str(prefix_str))
        except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
            return None

    @staticmethod
    def _is_valid_entry_name(name):
        return bool(name) and '/' not in name and '|' not in name

    def _fill_prefix_fields(self, data, prefix):
        data["prefix"] = str(prefix.cidr)
        data["prefixlen"] = prefix.prefixlen
        data["ipv"] = self.get_ip_type(prefix)

    def _get_cached_entry(self, key):
        return self.directory.get_slot(self.db_name, self.table_name).get(key)

    def _has_other_cached_entries(self, prefix_type, ipv, key):
        for cached_key, cached_data in self.directory.get_slot(self.db_name, self.table_name).items():
            if cached_key != key and cached_key.split('|', 1)[0] == prefix_type and cached_data.get("ipv") == ipv:
                return True
        return False

    def set_handler(self, key, data):
        log_debug("PrefixListMgr:: set handler")
        if '|' not in key:
            return True
        prefix_type, key_part = key.split('|', 1)
        data = dict(data)
        if data.get(PREFIX_FIELD):
            if not self._is_valid_entry_name(key_part):
                log_warn("PrefixListMgr:: Entry name '%s' is invalid for prefix list '%s': when the '%s' field is set, "
                         "the key must be a name without '/' or '|'" % (key_part, prefix_type, PREFIX_FIELD))
                return True
            prefix_str = data[PREFIX_FIELD]
        else:
            prefix_str = key_part
        prefix = self._parse_prefix(prefix_str)
        if prefix is None:
            log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_type))
            return True
        self._fill_prefix_fields(data, prefix)

        if PREFIX_TYPE_CONFIG.get(prefix_type) is None:
            # Remove a changed entry before adding its replacement so FRR cannot retain the old line.
            old_data = self._get_cached_entry(key)
            if old_data is not None:
                old_cmd = self._build_dynamic_prefix_list_cmd(prefix_type, old_data, add=True)
                new_cmd = self._build_dynamic_prefix_list_cmd(prefix_type, data, add=True)
                if old_cmd is not None and new_cmd is not None and old_cmd != new_cmd:
                    self.cfg_mgr.push(self._build_dynamic_prefix_list_cmd(prefix_type, old_data, add=False))
                    log_info("PrefixListMgr:: %s changed, removed the previous entry" % key)

        if self.generate_prefix_list_config(prefix_type, data, add=True, key=key):
            log_info("PrefixListMgr:: %s %s configuration generated" % (prefix_type, data["prefix"]))
            self.directory.put(self.db_name, self.table_name, key, data)
            log_info("PrefixListMgr:: set %s" % key)
        return True

    def del_handler(self, key):
        log_debug("PrefixListMgr:: del handler")
        if '|' not in key:
            return True
        prefix_type, key_part = key.split('|', 1)
        is_dynamic = PREFIX_TYPE_CONFIG.get(prefix_type) is None
        cached = self._get_cached_entry(key)
        # Without a cached prefix, even numeric or IP-looking names are not legacy CIDR keys.
        if not (cached and cached.get("prefix")) and '/' not in key_part:
            log_warn("PrefixListMgr:: No cached entry for '%s' in prefix list '%s'; skip delete" % (key_part, prefix_type))
            return True
        prefix_str = cached["prefix"] if cached and cached.get("prefix") else key_part
        prefix = self._parse_prefix(prefix_str)
        if prefix is None:
            log_warn("PrefixListMgr:: Prefix '%s' format is wrong for prefix list '%s'" % (prefix_str, prefix_type))
            return True

        data = dict(cached) if (is_dynamic and cached) else {}
        self._fill_prefix_fields(data, prefix)
        if is_dynamic and not data.get("action") and self._has_other_cached_entries(prefix_type, data["ipv"], key):
            # Delete-by-name would also wipe the other entries of this list.
            log_warn("PrefixListMgr:: Missing cached fields for '%s'; prefix list '%s' still has other entries, "
                     "skip best-effort delete-by-name" % (key_part, prefix_type))
            return True

        if self.generate_prefix_list_config(prefix_type, data, add=False):
            log_info("PrefixListMgr:: %s %s configuration deleted" % (prefix_type, data["prefix"]))
            if cached is not None:
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
