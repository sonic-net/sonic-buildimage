"""Validation shared by the single-instance SRv6 startup/runtime adapters."""

import ipaddress
import copy
import re


class ConfigError(ValueError):
    pass


MANAGED_TABLES = ('ISIS_GLOBALS', 'ISIS_GLOBALS_TIMERS', 'ISIS_INTERFACE',
                  'SRV6_MY_SOURCE', 'SRV6_MY_LOCATORS', 'SRV6_MY_SIDS')
ROUTED_TABLES = ('INTERFACE', 'PORTCHANNEL_INTERFACE', 'VLAN_INTERFACE',
                 'LOOPBACK_INTERFACE', 'VLAN_SUB_INTERFACE')


def table(config, name):
    rows = config.get(name, {})
    if not isinstance(rows, dict) or any(not isinstance(row, dict) for row in rows.values()):
        raise ConfigError('Invalid table snapshot: ' + name)
    return rows


def key_parts(key):
    parts = key if isinstance(key, tuple) else key.split('|') if isinstance(key, str) else ()
    if not parts or any(not isinstance(part, str) or not part for part in parts):
        raise ConfigError('Invalid configuration key')
    return parts


def identifier(value):
    if not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9_.:/-]+', value):
        raise ConfigError('Unsupported configuration identifier')
    return value


def integer(row, name, default):
    value = row.get(name, default)
    if isinstance(value, bool) or not re.fullmatch(r'[0-9]+', str(value)):
        raise ConfigError('Invalid numeric field: ' + name)
    return int(value)


def enabled(value):
    if str(value).lower() not in ('true', 'false'):
        raise ConfigError('Invalid boolean')
    return str(value).lower() == 'true'


def interfaces(config):
    names = set()
    for name in ('PORT', 'PORTCHANNEL', 'VLAN') + ROUTED_TABLES:
        for key in table(config, name):
            names.add(key_parts(key)[0])
    return names


def normalize_managed_config(config):
    """Copy a connector snapshot and serialize managed compound keys once."""
    result = copy.deepcopy(config)
    for name in MANAGED_TABLES:
        normalized = {}
        for key, row in table(result, name).items():
            serialized = '|'.join(key_parts(key))
            if serialized in normalized:
                raise ConfigError('Duplicate serialized configuration key')
            normalized[serialized] = row
        if name in result:
            result[name] = normalized
    return result


def _known_fields(row, fields):
    if set(row) - set(fields):
        raise ConfigError('Unsupported managed fields: ' + ','.join(sorted(set(row) - set(fields))))


def validate_isis_config(config):
    """Validate the adopted single default instance without requiring adjacency."""
    config = normalize_managed_config(config)
    globals_ = table(config, 'ISIS_GLOBALS')
    if any(key != 'default' for key in globals_):
        raise ConfigError('Recovery supports one default IS-IS instance')
    for row in globals_.values():
        _known_fields(row, ('enabled', 'net_title', 'level', 'ipv6_multi_topology',
                            'dynamic_hostname', 'srv6_locator', 'srv6_interface'))
        for field, default in (('enabled', 'false'), ('ipv6_multi_topology', 'true'),
                               ('dynamic_hostname', 'true')):
            enabled(row.get(field, default))
        if row.get('level', 'level-1-2') not in ('level-1', 'level-2', 'level-1-2'):
            raise ConfigError('Invalid IS-IS level')
        if 'net_title' in row:
            net = row['net_title']
            if not isinstance(net, str) or not re.fullmatch(r'[0-9a-fA-F]{2}(\.[0-9a-fA-F]{4}){3,9}\.00', net):
                raise ConfigError('Invalid NET')
    for key, row in table(config, 'ISIS_GLOBALS_TIMERS').items():
        _known_fields(row, ('lsp_mtu', 'lsp_refresh_interval', 'lsp_lifetime'))
        if key != 'default' or key not in globals_:
            raise ConfigError('Orphan IS-IS timer row')
        mtu, refresh, lifetime = [integer(row, name, default) for name, default in
                                  (('lsp_mtu', 1497), ('lsp_refresh_interval', 900), ('lsp_lifetime', 1200))]
        if not (128 <= mtu <= 4352 and 1 <= refresh <= 65235 and 350 <= lifetime <= 65535):
            raise ConfigError('Invalid IS-IS timer range')
        if lifetime < refresh + 300:
            raise ConfigError('IS-IS lifetime must exceed refresh by at least 300')
    known = interfaces(config)
    for name, row in table(config, 'ISIS_INTERFACE').items():
        identifier(name)
        _known_fields(row, ('enabled', 'ipv4_enabled', 'ipv6_enabled', 'passive', 'metric', 'circuit_type'))
        if name not in known or 'default' not in globals_:
            raise ConfigError('Orphan IS-IS interface')
        for field, default in (('enabled', 'false'), ('ipv4_enabled', 'true'),
                               ('ipv6_enabled', 'true'), ('passive', 'false')):
            enabled(row.get(field, default))
        if not 1 <= integer(row, 'metric', 10) <= 16777215:
            raise ConfigError('Invalid IS-IS metric')
        if row.get('circuit_type', 'lan') not in ('lan', 'p2p'):
            raise ConfigError('Invalid IS-IS circuit type')
        for table_name in ROUTED_TABLES:
            for key, entry in table(config, table_name).items():
                if len(key_parts(key)) == 1 and key_parts(key)[0] == name:
                    if entry.get('vrf_name', 'default') != 'default':
                        raise ConfigError('Recovery supports default-VRF interfaces only')


def validate_srv6_config(config, frr_version=None):
    """Reject an unrepresentable managed snapshot before rendering or mutation.

    Existing tables and the upstream ``adj`` spelling are retained. Validation
    does not query neighbors or claim programming readiness. The pinned 10.5
    function-length restriction is adapter-selected, not imposed on newer FRR.
    """
    if not isinstance(config, dict):
        raise ConfigError('Invalid configuration snapshot')
    config = normalize_managed_config(config)
    sources = table(config, 'SRV6_MY_SOURCE')
    if len(sources) > 1:
        raise ConfigError('SRv6 encapsulation source is a singleton')
    for key, row in sources.items():
        _known_fields(row, ('source-address',))
        if len(key_parts(key)) != 1:
            raise ConfigError('Invalid SRv6 source key')
        identifier(key_parts(key)[0])
        try:
            if ipaddress.IPv6Address(row['source-address']).is_unspecified:
                raise ValueError('An unspecified source is omitted by the native writer')
        except (KeyError, TypeError, ValueError) as error:
            raise ConfigError('Invalid SRv6 source address') from error

    locators = table(config, 'SRV6_MY_LOCATORS')
    for key, row in locators.items():
        _known_fields(row, ('prefix', 'block_len', 'node_len', 'func_len', 'arg_len',
                            'vrf', 'behavior', 'format'))
        if len(key_parts(key)) != 1:
            raise ConfigError('Invalid locator key')
        identifier(key_parts(key)[0])
        if 'argu_len' in row:
            raise ConfigError('Unsupported legacy argu_len spelling; use arg_len')
        block, node, function, argument = [integer(row, name, default) for name, default in
                                           (('block_len', 32), ('node_len', 16),
                                           ('func_len', 16), ('arg_len', 0))]
        if not (16 <= block <= 64 and 0 <= node <= 64 and 0 <= function <= 64):
            raise ConfigError('Unsupported locator geometry')
        if argument != 0 or row.get('vrf', 'default') != 'default':
            raise ConfigError('Locator arguments/nondefault VRF are not representable')
        if frr_version and tuple(frr_version[:2]) == (10, 5) and function > 20:
            raise ConfigError('FRR 10.5 supports at most 20 locator function bits')
        try:
            value = row['prefix']
            if not isinstance(value, str):
                raise ValueError('prefix is not a string')
            network = ipaddress.IPv6Network(
                value if '/' in value else value + '/' + str(block + node), strict=False)
        except (KeyError, TypeError, ValueError) as error:
            raise ConfigError('Invalid locator prefix') from error
        if network.prefixlen != block + node or network.prefixlen + function > 128:
            raise ConfigError('Locator prefix length does not match geometry')
        if row.get('behavior') not in (None, 'usid'):
            raise ConfigError('Unsupported locator behavior')
        if row.get('format') not in (None, 'usid-f3216'):
            raise ConfigError('Unsupported locator format')
        if row.get('format') and ((block, node, function, argument) != (32, 16, 16, 0) or
                                  row.get('behavior') != 'usid'):
            raise ConfigError('F3216 requires uSID behavior and 32/16/16/0 geometry')

    known_interfaces = interfaces(config)
    sid_owners = {}
    for key, row in table(config, 'SRV6_MY_SIDS').items():
        _known_fields(row, ('action', 'decap_vrf', 'decap_dscp_mode', 'interface', 'adj'))
        if row.get('decap_dscp_mode') not in (None, 'uniform', 'pipe'):
            raise ConfigError('Invalid decapsulation DSCP mode')
        parts = key_parts(key)
        if len(parts) != 2:
            raise ConfigError('Invalid static SID key')
        locator, prefix = parts
        if locator not in locators:
            raise ConfigError('SID references an absent locator')
        try:
            if '/' not in prefix:
                raise ValueError('SID requires prefix length')
            native_prefix = str(ipaddress.IPv6Network(prefix, strict=False))
        except ValueError as error:
            raise ConfigError('Invalid SID prefix') from error
        action = row.get('action')
        if action is None:
            continue
        if action not in ('uN', 'uDT4', 'uDT6', 'uDT46', 'uA'):
            raise ConfigError('Unsupported static SID action')
        if native_prefix in sid_owners:
            raise ConfigError('Conflicting native SID prefix: ' + native_prefix)
        sid_owners[native_prefix] = key
        if action == 'uA':
            if row.get('interface') not in known_interfaces:
                raise ConfigError('uA requires a configured interface')
            identifier(row['interface'])
            if 'adj' in row:
                try:
                    ipaddress.IPv6Address(row['adj'])
                except (TypeError, ValueError) as error:
                    raise ConfigError('Invalid uA adj address') from error
        elif 'interface' in row or 'adj' in row:
            raise ConfigError('Adjacency fields apply only to uA')
        if action in ('uDT4', 'uDT6', 'uDT46'):
            vrf = identifier(row.get('decap_vrf', 'default'))
            if vrf != 'default' and vrf not in table(config, 'VRF'):
                raise ConfigError('Unknown decapsulation VRF')

    for key, row in table(config, 'ISIS_GLOBALS').items():
        locator, interface = row.get('srv6_locator'), row.get('srv6_interface')
        if (locator is None) != (interface is None):
            raise ConfigError('IS-IS SRv6 locator/interface must be configured together')
        if locator is not None:
            if key != 'default' or locator not in locators or interface not in known_interfaces:
                raise ConfigError('Invalid default-instance SRv6 binding/reference')
            identifier(interface)
