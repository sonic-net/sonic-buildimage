from .log import log_debug, log_err, log_info
from .manager import Manager

FIB_ROUTE_FILTER_TABLE = "FIB_ROUTE_FILTER"
DEFAULT_VRF = "default"

# addr_family carries sonic-types:ip-family values ('IPv4'/'IPv6'); same
# mapping convention as bgpd.conf.db.pref_list.j2 / bgpd.conf.db.route_map.j2
# and the frrcfgd handler.
_IP_KEYWORD = {"IPv4": "ip", "IPv6": "ipv6"}


class FibRouteFilterMgr(Manager):
    """Runtime handler for FIB_ROUTE_FILTER in CONFIG_DB (separated mode).

    Each CONFIG_DB row binds a route-map to a (vrf, addr_family, protocol)
    tuple. This manager emits the matching FRR command via vtysh so the
    binding takes effect without a config_reload. The boot-time path is
    still owned by zebra.fib_route_filter.conf.j2.

    CONFIG_DB key shape: "<vrf>|<addr_family>|<protocol>" (e.g.
    "default|IPv4|bgp"). Field: route_map.

    Emitted commands:
        <ip|ipv6> protocol <proto> route-map <name>      (set, default VRF)
        vrf <name>                                       (set, named VRF)
         <ip|ipv6> protocol <proto> route-map <name>
        exit-vrf
        no <ip|ipv6> protocol <proto>                    (delete)

    A per-key state cache lets deletes emit the `no ...` form without
    needing the prior route_map field, and lets idempotent re-sets short-
    circuit.
    """

    def __init__(self, common_objs, db, table):
        super(FibRouteFilterMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )
        # frf_key (str) -> last applied route_map name
        self._applied = {}

    def set_handler(self, key, data):
        parsed = self._parse_key(key)
        if parsed is None:
            return True
        entry_vrf, afi, protocol = parsed
        ip_kw = _IP_KEYWORD.get(afi)
        if ip_kw is None:
            log_err("FibRouteFilterMgr: unsupported addr_family '%s' in key '%s'" % (afi, key))
            return True

        route_map = data.get("route_map") if isinstance(data, dict) else None
        if not route_map:
            log_err("FibRouteFilterMgr: missing route_map for key '%s'" % key)
            return True

        if self._applied.get(key) == route_map:
            log_debug("FibRouteFilterMgr: %s already bound to %s; skipping" % (key, route_map))
            return True

        # FRR's `ip|ipv6 protocol X route-map Y` is upsert-style: emitting
        # the new binding replaces any prior route-map for the same
        # (vrf, afi, proto), so we don't need to first emit a `no ...`.
        cmds = self._build_set_cmds(entry_vrf, ip_kw, protocol, route_map)
        log_info("FibRouteFilterMgr: binding %s -> %s" % (key, route_map))
        self.cfg_mgr.push_list(cmds)
        self._applied[key] = route_map
        return True

    def del_handler(self, key):
        parsed = self._parse_key(key)
        if parsed is None:
            return
        entry_vrf, afi, protocol = parsed

        # Fast-exit before the AFI check: a key with an unsupported AFI
        # can't have been pushed by set_handler, so it's not in _applied
        # and there's nothing to undo. Without this ordering, a DEL on
        # such a key would emit a spurious unsupported-AFI error.
        if key not in self._applied:
            log_debug("FibRouteFilterMgr: del for untracked key '%s'; skipping" % key)
            return

        ip_kw = _IP_KEYWORD.get(afi)
        if ip_kw is None:
            log_err("FibRouteFilterMgr: unsupported addr_family '%s' in key '%s'" % (afi, key))
            return

        cmds = self._build_del_cmds(entry_vrf, ip_kw, protocol)
        log_info("FibRouteFilterMgr: removing binding %s" % key)
        self.cfg_mgr.push_list(cmds)
        self._applied.pop(key, None)

    @staticmethod
    def _parse_key(key):
        # swsscommon delivers composite keys as '|'-joined strings (same as
        # PrefixListMgr). Defensive: also accept tuples in case the runner
        # ever changes.
        if isinstance(key, tuple):
            parts = list(key)
        else:
            parts = str(key).split("|")
        if len(parts) != 3 or not all(parts):
            log_err("FibRouteFilterMgr: malformed key '%s' (expected vrf|afi|protocol)" % str(key))
            return None
        return parts[0], parts[1], parts[2]

    @staticmethod
    def _build_set_cmds(vrf, ip_kw, protocol, route_map):
        line = "%s protocol %s route-map %s" % (ip_kw, protocol, route_map)
        if vrf == DEFAULT_VRF:
            return [line]
        return ["vrf %s" % vrf, " %s" % line, "exit-vrf"]

    @staticmethod
    def _build_del_cmds(vrf, ip_kw, protocol):
        line = "no %s protocol %s" % (ip_kw, protocol)
        if vrf == DEFAULT_VRF:
            return [line]
        return ["vrf %s" % vrf, " %s" % line, "exit-vrf"]
