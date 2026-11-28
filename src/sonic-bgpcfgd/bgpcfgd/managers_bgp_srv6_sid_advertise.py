import re

import jinja2
from swsscommon import swsscommon

from .log import log_err, log_info, log_warn
from .manager import Manager


class BGPSrv6SidAdvertiseMgr(Manager):
    """Configure BGP SRv6 SID advertisement from BGP_DEVICE_GLOBAL|SRV6."""

    ADVERTISE_KEY = "advertise"
    PREFIX_LIST_KEY = "prefix_list"
    BEHAVIOR = "dt46"
    AF_CONFIG = {
        "ipv4_unicast": {
            "address_family": "ipv4",
            "route_map": "BGP_SRV6_ADVERTISE_IPV4",
            "advertise_field": "bgp_srv6_ipv4_sid_advertise",
            "prefix_list_field": "bgp_srv6_ipv4_sid_prefix_list",
        },
        "ipv6_unicast": {
            "address_family": "ipv6",
            "route_map": "BGP_SRV6_ADVERTISE_IPV6",
            "advertise_field": "bgp_srv6_ipv6_sid_advertise",
            "prefix_list_field": "bgp_srv6_ipv6_sid_prefix_list",
        },
    }
    PREFIX_LIST_RE = re.compile(r"^[A-Za-z0-9_.-]{1,63}$")

    def __init__(self, common_objs, db, table):
        deps = [("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn")]
        super(BGPSrv6SidAdvertiseMgr, self).__init__(common_objs, deps, db, table)
        self.config = {}
        self.sid_advertise_template = common_objs["tf"].from_file("bgpd/srv6/bgpd.srv6.sid_advertise.conf.j2")
        metadata = self.directory.get_path(
            "CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost") or {}
        self.bgp_asn = metadata.get("bgp_asn")
        self.directory.subscribe(deps, self.on_bgp_asn_change)

    def set_handler(self, key, data):
        if key != "SRV6":
            return True

        self.update_bgp_asn()
        field_map = {}
        for afi_safi, af_config in self.AF_CONFIG.items():
            field_map[af_config["advertise_field"]] = (afi_safi, self.ADVERTISE_KEY)
            field_map[af_config["prefix_list_field"]] = (afi_safi, self.PREFIX_LIST_KEY)

        updates = {}
        for field, value in data.items():
            if field in field_map:
                afi_safi, config_key = field_map[field]
                updates.setdefault(afi_safi, {})[config_key] = value

        for afi_safi, af_updates in updates.items():
            previous = self.config.get(afi_safi, {self.ADVERTISE_KEY: "false"})
            desired = dict(previous)
            desired.update(af_updates)
            advertise = desired.get(self.ADVERTISE_KEY, "false")
            if advertise not in ("true", "false", True, False):
                log_warn("BGPSrv6SidAdvertiseMgr:: invalid advertisement value %r for BGP_DEVICE_GLOBAL|SRV6/%s" %
                         (advertise, self.AF_CONFIG[afi_safi]["advertise_field"]))
                continue

            prefix_list = desired.get(self.PREFIX_LIST_KEY)
            if prefix_list and (not isinstance(prefix_list, str) or not self.PREFIX_LIST_RE.match(prefix_list)):
                log_warn("BGPSrv6SidAdvertiseMgr:: invalid prefix list value %r for BGP_DEVICE_GLOBAL|SRV6/%s" %
                         (prefix_list, self.AF_CONFIG[afi_safi]["prefix_list_field"]))
                continue

            enabled = advertise is True or advertise == "true"
            if not prefix_list:
                desired.pop(self.PREFIX_LIST_KEY, None)
            desired[self.ADVERTISE_KEY] = "true" if enabled else "false"
            if self.config.get(afi_safi) == desired:
                continue

            self.apply_config(afi_safi, desired, previous)
            self.config[afi_safi] = desired

        return True

    def del_handler(self, key):
        if key != "SRV6":
            return
        for afi_safi in self.AF_CONFIG:
            previous = self.config.pop(afi_safi, None)
            self.apply_config(afi_safi, {self.ADVERTISE_KEY: "false"}, previous)

    def update_bgp_asn(self):
        metadata = self.directory.get_path("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost") or {}
        if metadata.get("bgp_asn"):
            self.bgp_asn = metadata["bgp_asn"]

    def on_bgp_asn_change(self):
        self.update_bgp_asn()
        if not self.bgp_asn:
            return
        for afi_safi, config in self.config.items():
            self.apply_config(afi_safi, config)

    def apply_config(self, afi_safi, desired, previous=None):
        self.update_bgp_asn()
        if not self.bgp_asn:
            return
        af_config = self.AF_CONFIG[afi_safi]
        address_family = af_config["address_family"]
        route_map = af_config["route_map"]
        old_prefix_list = previous.get(self.PREFIX_LIST_KEY) if previous else None
        new_prefix_list = desired.get(self.PREFIX_LIST_KEY)
        enabled = desired.get(self.ADVERTISE_KEY) == "true"
        try:
            command = self.sid_advertise_template.render(
                bgp_asn=self.bgp_asn,
                address_family=address_family,
                behavior=self.BEHAVIOR,
                advertise=enabled,
                prefix_list=new_prefix_list,
                route_map=route_map,
                cleanup_route_map=bool(old_prefix_list or new_prefix_list),
            ).strip()
        except jinja2.TemplateError as e:
            log_err("BGPSrv6SidAdvertiseMgr:: failed to render SID advertisement configuration for %s: %s" %
                    (afi_safi, str(e)))
            return

        if command:
            self.cfg_mgr.push("\n" + command + "\n")
            log_info("BGPSrv6SidAdvertiseMgr:: scheduled %s SID advertisement for %s" %
                     ("enabled" if enabled else "disabled", afi_safi))
