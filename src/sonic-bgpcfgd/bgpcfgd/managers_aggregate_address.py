import ipaddress
import re

from swsscommon import swsscommon

from .log import log_info, log_err
from .manager import Manager
from .managers_bbr import BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED

CONFIG_DB_NAME = "CONFIG_DB"
BGP_AGGREGATE_ADDRESS_TABLE_NAME = "BGP_AGGREGATE_ADDRESS"
BBR_REQUIRED_KEY = "bbr-required"
AS_SET_KEY = "as-set"
SUMMARY_ONLY_KEY = "summary-only"
AGGREGATE_ADDRESS_PREFIX_LIST_KEY = "aggregate-address-prefix-list"
CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY = "contributing-address-prefix-list"
COMMON_TRUE_STRING = "true"
COMMON_FALSE_STRING = "false"
ADDRESS_STATE_KEY = "state"
ADDRESS_ACTIVE_STATE = "active"
ADDRESS_INACTIVE_STATE = "inactive"
_PREFIX_LIST_RE = re.compile(
    r"^(no )?(ip|ipv6) prefix-list (\S+)(?: seq ([0-9]+))?"
    r"(?: (permit|deny) (\S+)((?: (?:ge|le) [0-9]+)*))?$"
)


class AggregateAddressMgr(Manager):
    """ This class is to subscribe BGP_AGGREGATE_ADDRESS in CONFIG_DB """

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(AggregateAddressMgr, self).__init__(
            common_objs,
            [
                ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),
            ],
            db,
            table,
        )
        self.directory.subscribe([(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)], self.on_bbr_change)
        self.state_db_conn = common_objs['state_db_conn']
        self.address_table = swsscommon.Table(self.state_db_conn, BGP_AGGREGATE_ADDRESS_TABLE_NAME)
        self.remove_all_state_of_address()

    def on_bbr_change(self):
        bbr_status = self.directory.get(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)
        addresses = self.get_addresses_from_state_db(bbr_required_only=True)
        if bbr_status == BGP_BBR_STATUS_ENABLED:
            log_info("AggregateAddressMgr::BBR state changed to %s with bbr_required addresses %s" % (bbr_status, addresses))
            for address in addresses:
                if self.address_set_handler(address[0], address[1]):
                    self.set_address_state(address[0], address[1], ADDRESS_ACTIVE_STATE)
                else:
                    log_info("AggregateAddressMgr::set address %s failed during BBR change (validation or FRR push error)" % key2prefix(address[0]))
                    self.set_address_state(address[0], address[1], ADDRESS_INACTIVE_STATE)
        elif bbr_status == BGP_BBR_STATUS_DISABLED:
            log_info("AggregateAddressMgr::BBR state changed to %s with bbr_required addresses %s" % (bbr_status, addresses))
            inactive_addresses = []
            for address in addresses:
                address_state = address[1]
                if address_state.get(ADDRESS_STATE_KEY) == ADDRESS_INACTIVE_STATE:
                    inactive_addresses.append(address)
                    continue
                if self.address_del_handler(address[0], address_state):
                    self.set_address_state(address[0], address[1], ADDRESS_INACTIVE_STATE)
            if inactive_addresses:
                snapshot_state = self._build_effective_state()
                if snapshot_state is not None:
                    for address in inactive_addresses:
                        self._reconcile_inactive_address(snapshot_state, address[0], address[1])
        else:
            log_info("AggregateAddressMgr::BBR state changed to unknown with bbr_required addresses %s" % addresses)

    def set_handler(self, key, data):
        data = dict(data)
        prefix = key2prefix(key)
        net, reason = validate_prefix(prefix)
        if net is None:
            log_err("AggregateAddressMgr::invalid aggregate prefix %s: %s" % (prefix, reason))
            self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
            return True
        if self.directory.path_exist(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY):
            bbr_status = self.directory.get(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)
        else:
            bbr_status = ""
        bbr_required = data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING) == COMMON_TRUE_STRING
        if bbr_status not in (BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED) and bbr_required:
            log_info("AggregateAddressMgr::BBR state is unknown and bbr-required is true. Skip the address %s" % prefix)
            self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        elif bbr_status == BGP_BBR_STATUS_DISABLED and bbr_required:
            log_info("AggregateAddressMgr::BBR is disabled and bbr-required is set to true. Skip the address %s" % prefix)
            self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        else:
            if self.address_set_handler(key, data):
                self.set_address_state(key, data, ADDRESS_ACTIVE_STATE)
            else:
                log_info("AggregateAddressMgr::set address %s failed (validation or FRR push error)" % prefix)
                self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        return True

    def address_set_handler(self, key, data):
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        prefix = key2prefix(key)

        net, reason = validate_prefix(prefix)
        if net is None:
            log_err("AggregateAddressMgr::invalid aggregate prefix %s: %s" % (prefix, reason))
            return False

        is_v4 = net.version == 4
        cmd_list = []

        aggregates_cmds = generate_aggregate_address_commands(
            asn=bgp_asn,
            prefix=prefix,
            is_v4=is_v4,
            is_remove=False,
            summary_only=data.get(SUMMARY_ONLY_KEY, COMMON_FALSE_STRING),
            as_set=data.get(AS_SET_KEY, COMMON_FALSE_STRING)
        )
        cmd_list.extend(aggregates_cmds)

        if AGGREGATE_ADDRESS_PREFIX_LIST_KEY in data and data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY]:
            append_agg_address_cmd = generate_prefix_list_commands(
                prefix_list_name=data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=False,
                is_remove=False
            )
            cmd_list.extend(append_agg_address_cmd)

        if CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY in data and data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY]:
            append_con_address_cmd = generate_prefix_list_commands(
                prefix_list_name=data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=True,
                is_remove=False
            )
            cmd_list.extend(append_con_address_cmd)

        log_info("AggregateAddressMgr::cmd_list: %s" % cmd_list)
        self.cfg_mgr.push_list(cmd_list)
        return True

    def del_handler(self, key):
        address_state = self.get_address_from_state_db(key)
        if address_state.get(ADDRESS_STATE_KEY) == ADDRESS_INACTIVE_STATE:
            log_info("AggregateAddressMgr::address %s is inactive, skip FRR removal" % key2prefix(key))
        else:
            if self.address_del_handler(key, address_state):
                log_info("AggregateAddressMgr::delete address %s success" % key)
        self.del_address_state(key)
        return True

    def address_del_handler(self, key, data):
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        prefix = key2prefix(key)
        net, _ = validate_prefix(prefix)
        is_v4 = net.version == 4 if net else False
        cmd_list = []

        aggregates_cmds = generate_aggregate_address_commands(
            asn=bgp_asn,
            prefix=prefix,
            is_v4=is_v4,
            is_remove=True
        )
        cmd_list.extend(aggregates_cmds)

        if AGGREGATE_ADDRESS_PREFIX_LIST_KEY in data and data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY]:
            rm_agg_address_cmds = generate_prefix_list_commands(
                prefix_list_name=data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=False,
                is_remove=True
            )
            cmd_list.extend(rm_agg_address_cmds)

        if CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY in data and data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY]:
            rm_con_address_cmds = generate_prefix_list_commands(
                prefix_list_name=data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=True,
                is_remove=True
            )
            cmd_list.extend(rm_con_address_cmds)

        log_info("AggregateAddressMgr::cmd_list: %s" % cmd_list)
        self.cfg_mgr.push_list(cmd_list)
        return True

    def _build_effective_state(self):
        self.cfg_mgr.update()
        running_lines = self.cfg_mgr.get_text()
        if not any(line.strip() for line in running_lines):
            log_err("AggregateAddressMgr::FRR snapshot is unavailable or empty, skip inactive reconciliation")
            return None
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        normalized_asn = _normalize_asn(bgp_asn)
        state = {"aggregates": set(), "prefix_rules": set()}
        if (
            normalized_asn is None or
            not _apply_config_lines(state, running_lines, normalized_asn) or
            not _apply_config_lines(state, self.cfg_mgr.changes.splitlines(), normalized_asn)
        ):
            log_err("AggregateAddressMgr::FRR snapshot or pending changes are invalid, skip inactive reconciliation")
            return None
        return state

    def _reconcile_inactive_address(self, state, key, data):
        prefix = key2prefix(key)
        net, reason = validate_prefix(prefix)
        if net is None:
            log_err("AggregateAddressMgr::invalid aggregate prefix %s: %s" % (prefix, reason))
            return
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        is_v4 = net.version == 4
        af = "ipv4" if is_v4 else "ipv6"
        family = "ip" if is_v4 else "ipv6"
        cmd_list = []
        normalized_prefix = str(net)
        if (af, normalized_prefix) in state["aggregates"]:
            cmd_list.extend(generate_aggregate_address_commands(asn=bgp_asn, prefix=prefix, is_v4=is_v4, is_remove=True))
        prefix_lists = (
            (data.get(AGGREGATE_ADDRESS_PREFIX_LIST_KEY, ""), None, False),
            (data.get(CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY, ""), 32 if is_v4 else 128, True),
        )
        for name, le, is_con in prefix_lists:
            if name and _prefix_rule_exists(state, family, name, normalized_prefix, le):
                cmd_list.extend(generate_prefix_list_commands(name, prefix, is_v4, is_con, True))
        if cmd_list:
            log_info("AggregateAddressMgr::inactive address %s reconciliation cmd_list: %s" % (prefix, cmd_list))
            self.cfg_mgr.push_list(cmd_list)
            _apply_config_lines(state, cmd_list, _normalize_asn(bgp_asn))

    def get_addresses_from_state_db(self, bbr_required_only=False):
        addresses = []
        for key in self.address_table.getKeys():
            data = self.get_address_from_state_db(key)
            if not bbr_required_only or data[BBR_REQUIRED_KEY] == COMMON_TRUE_STRING:
                addresses.append((key, data))
        return addresses

    def get_address_from_state_db(self, key):
        (success, data) = self.address_table.get(key)
        if not success:
            log_err("AggregateAddressMgr::Failed to get data from state db for key %s" % key)
            return {}
        data = dict(data)
        return data

    def remove_all_state_of_address(self):
        for address in list(self.address_table.getKeys()):
            self.address_table.delete(address)
        log_info("AggregateAddressMgr::All the state of aggregate address is removed")
        return True

    def set_address_state(self, key, data, address_state):
        self.address_table.hset(key, BBR_REQUIRED_KEY, data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, SUMMARY_ONLY_KEY, data.get(SUMMARY_ONLY_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, AS_SET_KEY, data.get(AS_SET_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, AGGREGATE_ADDRESS_PREFIX_LIST_KEY, data.get(AGGREGATE_ADDRESS_PREFIX_LIST_KEY, ""))
        self.address_table.hset(key, CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY, data.get(CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY, ""))
        self.address_table.hset(key, ADDRESS_STATE_KEY, address_state)
        log_info("AggregateAddressMgr::State of aggregate address %s is set with bbr_required %s and state %s " % (key, data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING), address_state))

    def del_address_state(self, key):
        self.address_table.delete(key)
        log_info("AggregateAddressMgr::State of aggregate address %s is removed" % key)


def _apply_config_lines(state, lines, bgp_asn):
    current_router = False
    current_af = None
    for raw_line in lines:
        line = raw_line.strip()
        if not line or line.startswith("!"):
            continue
        tokens = line.split()
        if tokens[:2] == ["router", "bgp"]:
            if len(tokens) < 3:
                return False
            router_asn = _normalize_asn(tokens[2])
            if router_asn is None:
                return False
            current_router = len(tokens) == 3 and router_asn == bgp_asn
            current_af = None
            continue
        prefix_list = _apply_prefix_list_line(state, " ".join(tokens))
        if prefix_list is not None:
            if not prefix_list:
                return False
            continue
        if tokens[0] in ("router", "end", "exit"):
            current_router = False
            current_af = None
            continue
        if tokens[0] == "address-family":
            current_af = None
            if current_router and tokens[1:] in (
                ["ipv4"], ["ipv4", "unicast"], ["ipv6"], ["ipv6", "unicast"]
            ):
                current_af = tokens[1]
            continue
        if line == "exit-address-family":
            current_af = None
            continue
        remove = tokens[0] == "no"
        idx = 1 if remove else 0
        if current_router and current_af and tokens[idx:idx + 1] == ["aggregate-address"]:
            if len(tokens) < idx + 2:
                return False
            prefix = _normalize_prefix(tokens[idx + 1], current_af)
            if prefix is None:
                return False
            if remove:
                state["aggregates"].discard((current_af, prefix))
            else:
                state["aggregates"].add((current_af, prefix))
    return True


def _apply_prefix_list_line(state, line):
    tokens = line.split()
    if tokens[:1] == ["no"]:
        tokens = tokens[1:]
    if tokens[:2] in (["ip", "prefix-list"], ["ipv6", "prefix-list"]):
        if tokens[2:] == ["sequence-number"] or tokens[3:4] == ["description"]:
            return True
    match = _PREFIX_LIST_RE.fullmatch(line)
    if match is None:
        if line.startswith(("ip prefix-list", "ipv6 prefix-list", "no ip prefix-list", "no ipv6 prefix-list")):
            return False
        return None
    remove, family, name, seq, action, prefix, modifiers = match.groups()
    if action is None and not remove:
        return False
    rules = state["prefix_rules"]
    seq = int(seq) if seq is not None else None
    # Unsequenced queued additions do not reveal which sequence FRR will assign.
    if seq is not None and any(rule[:3] == (family, name, None) for rule in rules):
        return False
    if action is None:
        state["prefix_rules"] = {
            rule for rule in rules
            if rule[:2] != (family, name) or (seq is not None and rule[2] != seq)
        }
        return True
    prefix = "any" if prefix == "any" else _normalize_prefix(prefix, "ipv4" if family == "ip" else "ipv6")
    if prefix is None:
        return False
    pairs = re.findall(r"(ge|le) ([0-9]+)", modifiers or "")
    if len({name for name, _ in pairs}) != len(pairs):
        return False
    bounds = {name: int(value) for name, value in pairs}
    rule = (family, name, seq, action, prefix, bounds.get("ge"), bounds.get("le"))
    if remove:
        if seq is not None:
            rules = {current for current in rules if current[:3] != (family, name, seq)}
        else:
            matches = [
                current for current in rules
                if current[:2] == (family, name) and current[3:] == rule[3:]
            ]
            if matches:
                rules.remove(min(matches, key=lambda current: current[2] or 4294967296))
    elif seq is not None:
        rules = {current for current in rules if current[:3] != (family, name, seq)}
        rules.add(rule)
    elif not any(current[:2] == (family, name) and current[3:] == rule[3:] for current in rules):
        rules.add(rule)
    state["prefix_rules"] = rules
    return True


def _prefix_rule_exists(state, family, name, prefix, le):
    return any(
        rule[:2] == (family, name) and rule[3:] == ("permit", prefix, None, le)
        for rule in state["prefix_rules"]
    )


def _normalize_prefix(prefix, af):
    if "/" not in prefix:
        return None
    try:
        net = ipaddress.ip_network(prefix, strict=True)
    except ValueError:
        return None
    return str(net) if net.version == (4 if af == "ipv4" else 6) else None


def _normalize_asn(asn):
    parts = str(asn).split(".")
    if not all(part.isdigit() for part in parts) or len(parts) not in (1, 2):
        return None
    if len(parts) == 2 and any(int(part) > 65535 for part in parts):
        return None
    value = int(parts[0]) if len(parts) == 1 else int(parts[0]) * 65536 + int(parts[1])
    return value if 0 < value <= 4294967295 else None


def key2prefix(key):
    prefix = key.split("|")[-1]
    return prefix


def validate_prefix(prefix):
    """Return (network, None) if prefix is valid, or (None, reason) otherwise."""
    if '/' not in prefix:
        return None, "missing prefix length"
    try:
        net = ipaddress.ip_network(prefix, strict=True)
    except ValueError as e:
        return None, str(e)
    return net, None


def generate_aggregate_address_commands(asn, prefix, is_v4, is_remove, summary_only=COMMON_FALSE_STRING, as_set=COMMON_FALSE_STRING):
    ret_cmds = []
    ret_cmds.append("router bgp %s" % asn)
    ret_cmds.append("address-family ipv4" if is_v4 else "address-family ipv6")
    agg_cmd = "no " if is_remove else ""
    agg_cmd += "aggregate-address %s" % prefix
    if not is_remove and summary_only == COMMON_TRUE_STRING:
        agg_cmd += " %s" % SUMMARY_ONLY_KEY
    if not is_remove and as_set == COMMON_TRUE_STRING:
        agg_cmd += " %s" % AS_SET_KEY
    ret_cmds.append(agg_cmd)
    ret_cmds.append("exit-address-family")
    ret_cmds.append("exit")
    return ret_cmds


def generate_prefix_list_commands(prefix_list_name, prefix, is_v4, is_con, is_remove):
    ret_cmds = []
    prefix_list_cmd = "no " if is_remove else ""
    prefix_list_cmd += "ip" if is_v4 else "ipv6"
    prefix_list_cmd += " prefix-list %s" % prefix_list_name
    prefix_list_cmd += " permit %s" % prefix
    if is_con:
        prefix_list_cmd += " le" + (" 32" if is_v4 else " 128")
    ret_cmds.append(prefix_list_cmd)
    return ret_cmds
