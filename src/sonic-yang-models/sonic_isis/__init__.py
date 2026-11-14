"""IS-IS northbound schema semantics shared by CLI and the FRR adapter.

Shared identities/defaults and atomic ConfigDB graph transactions; no daemon I/O.
"""
import copy
import re

TABLES = ('ISIS_INSTANCE', 'ISIS_INSTANCE_LEVEL', 'ISIS_INTERFACE',
          'ISIS_INTERFACE_LEVEL', 'ISIS_REDISTRIBUTE')
ROUTED_TABLES = ('INTERFACE', 'PORTCHANNEL_INTERFACE', 'VLAN_INTERFACE',
                 'LOOPBACK_INTERFACE', 'VLAN_SUB_INTERFACE')
LEVELS = ('level-1', 'level-2')
LEVEL_TYPES = ('level-1', 'level-2-only', 'level-1-2')
BACKOFF = ('spf_init_delay', 'spf_short_delay', 'spf_long_delay', 'spf_hold_down', 'spf_time_to_learn')
# (default, validator). None means optional/absent, not an invented FRR default.


def boolean(value):
    if value not in ('true', 'false'):
        raise ValueError('expected true or false')
    return value


def number(low, high):
    def check(value):
        if not re.fullmatch(r'[0-9]+', str(value)) or not low <= int(value) <= high:
            raise ValueError('expected integer {}..{}'.format(low, high))
        return str(int(value))
    return check


def choice(*values):
    def check(value):
        if value not in values:
            raise ValueError('expected one of ' + ', '.join(values))
        return value
    return check


def identifier(value):
    if not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_.-]{0,63}', value):
        raise ValueError('invalid identifier')
    return value


FIELDS = {
    'ISIS_INSTANCE': {
        'enabled': ('false', boolean), 'level_type': ('level-1-2', choice(*LEVEL_TYPES)),
        'dynamic_hostname': ('true', boolean), 'metric_style': ('wide', choice('wide')),
        'lsp_mtu': ('1497', number(128, 4352)), 'overload': ('false', boolean),
        'overload_on_startup': ('0', number(0, 86400)), 'ipv6_multi_topology': ('false', boolean),
        **{field: (None, number(0, 60000)) for field in BACKOFF},
    },
    'ISIS_INSTANCE_LEVEL': {
        'lsp_refresh_interval': ('900', number(1, 65235)),
        'lsp_maximum_lifetime': ('1200', number(350, 65535)),
        'lsp_generation_interval': ('30', number(1, 120)),
        'spf_minimum_interval': ('1', number(1, 120)),
    },
    'ISIS_INTERFACE': {
        'vrf': (None, identifier), 'instance': (None, identifier),
        'enabled': ('false', boolean), 'ipv4_enabled': ('false', boolean),
        'ipv6_enabled': ('false', boolean), 'passive': ('false', boolean),
        'network_type': ('broadcast', choice('broadcast', 'point-to-point')),
        'level_type': ('level-1-2', choice(*LEVEL_TYPES)),
    },
    'ISIS_INTERFACE_LEVEL': {
        'metric': ('10', number(0, 16777215)), 'hello_interval': ('3', number(1, 600)),
        'hello_multiplier': ('10', number(2, 100)),
    },
    'ISIS_REDISTRIBUTE': {'metric': ('0', number(0, 16777215)), 'route_map': (None, identifier)},
}
PROTOCOLS = {'ipv4': ('connected', 'static', 'kernel', 'bgp', 'ospf', 'rip'),
             'ipv6': ('connected', 'static', 'kernel', 'bgp', 'ospf6', 'ripng')}


def key_tuple(key):
    return key if isinstance(key, tuple) else tuple(key.split('|'))


def normalize(tables):
    result = copy.deepcopy(tables)
    for table in TABLES:
        result[table] = {key_tuple(k) if table not in ('ISIS_INTERFACE',) else k: v
                         for k, v in tables.get(table, {}).items()}
    return result


def validate_transition(before, after):
    """Reject live area removal that corrupts sibling areas in pinned FRR.

    FRR 10.7.1 clears a VRF's shared system ID when destroying an area. Keep
    multi-area creation available, but do not silently rebuild surviving areas.
    An explicit atomic shutdown of all old areas has no surviving sibling.
    """
    old = {key for key, row in normalize(before)['ISIS_INSTANCE'].items() if row.get('enabled') == 'true'}
    new = {key for key, row in normalize(after)['ISIS_INSTANCE'].items() if row.get('enabled') == 'true'}
    survivors = {key[0] for key in old & new}
    if any(key[0] in survivors for key in old - new):
        raise ValueError('Pinned FRR cannot safely delete/disable an active area while another active area '
                         'in the same VRF survives; operation refused until FRR is fixed')


def effective(table, row):
    return {**{k: default for k, (default, _) in FIELDS[table].items() if default is not None}, **row}


def routed_vrf(tables, name):
    for table in ROUTED_TABLES:
        if name in tables.get(table, {}):
            return tables[table][name].get('vrf_name', 'default')
    return None


def net(value):
    if not isinstance(value, str) or not re.fullmatch(r'[0-9a-fA-F]{2}(\.[0-9a-fA-F]{4}){3,9}\.00', value):
        raise ValueError('invalid NET (system ID and zero NSEL required)')
    return value.lower()


def validate(tables):
    """Validate the complete candidate graph before either CLI writes or native I/O."""
    data = normalize(tables)
    instances = data['ISIS_INSTANCE']
    lengths = dict(zip(TABLES, (2, 3, 1, 2, 5)))
    for table in TABLES:
        for key, row in data[table].items():
            parts = (key,) if table == 'ISIS_INTERFACE' else key
            if len(parts) != lengths[table]:
                raise ValueError('invalid {} key {}'.format(table, key))
            for part in parts:
                identifier(part)
            for field, value in row.items():
                if table == 'ISIS_INSTANCE' and field == 'net_titles':
                    if not isinstance(value, list) or len(value) > 3 or len(set(value)) != len(value):
                        raise ValueError('NET must be a unique list of at most three values')
                    row[field] = [net(item) for item in value]
                    if len(set(row[field])) != len(row[field]):
                        raise ValueError('duplicate NET')
                elif field not in FIELDS[table]:
                    raise ValueError('unknown {} field {}'.format(table, field))
                else:
                    try:
                        row[field] = FIELDS[table][field][1](value)
                    except ValueError as error:
                        raise ValueError('{} {}: {}'.format(table, field, error)) from error
    system_ids = {}
    for (vrf, instance), row in instances.items():
        if vrf != 'default' and vrf not in data.get('VRF', {}):
            raise ValueError('VRF {} does not exist'.format(vrf))
        if row.get('enabled') == 'true' and not row.get('net_titles'):
            raise ValueError('an enabled instance requires a NET')
        for address in row.get('net_titles', []):
            system_id = address.replace('.', '')[-14:-2]
            if system_ids.setdefault(vrf, system_id) != system_id:
                raise ValueError('instances in the same VRF must share the FRR system ID')
        present = set(BACKOFF) & row.keys()
        if present and present != set(BACKOFF):
            raise ValueError('all five SPF backoff values are required together')
        if present and int(row['spf_hold_down']) <= int(row['spf_time_to_learn']):
            raise ValueError('SPF hold-down must exceed time-to-learn')
    for (vrf, instance, level), row in data['ISIS_INSTANCE_LEVEL'].items():
        if (vrf, instance) not in instances or level not in LEVELS:
            raise ValueError('invalid instance/level reference')
        values = effective('ISIS_INSTANCE_LEVEL', row)
        if int(values['lsp_maximum_lifetime']) < int(values['lsp_refresh_interval']) + 300:
            raise ValueError('LSP lifetime must be at least refresh + 300')
        if int(values['lsp_generation_interval']) >= int(values['lsp_refresh_interval']):
            raise ValueError('LSP generation interval must be less than refresh')
    for name, row in data['ISIS_INTERFACE'].items():
        if (row.get('vrf'), row.get('instance')) not in instances:
            raise ValueError('interface {} requires an explicit valid parent'.format(name))
        # A routed-interface removal/move can temporarily invalidate this reference.
        # Runtime must detach it, not guess another parent or block other instances.
    for (name, level), row in data['ISIS_INTERFACE_LEVEL'].items():
        if name not in data['ISIS_INTERFACE'] or level not in LEVELS:
            raise ValueError('invalid interface/level reference')
    for (vrf, instance, af, protocol, level), row in data['ISIS_REDISTRIBUTE'].items():
        if (vrf, instance) not in instances or protocol not in PROTOCOLS.get(af, ()) or level not in LEVELS:
            raise ValueError('invalid redistribution key or reference')
        parent_level = effective('ISIS_INSTANCE', instances[(vrf, instance)])['level_type']
        if parent_level != 'level-1-2' and level != parent_level.replace('-only', ''):
            raise ValueError('redistribution level is disabled on its parent')
        if row.get('route_map') and row['route_map'] not in data.get('ROUTE_MAP_SET', {}):
            raise ValueError('redistribution route-map does not exist')
    return data


def active_interface(data, name):
    row = effective('ISIS_INTERFACE', data['ISIS_INTERFACE'][name])
    parent = data['ISIS_INSTANCE'].get((row['vrf'], row['instance']), {})
    member = any(key_tuple(key)[-1] == name for table in ('PORTCHANNEL_MEMBER', 'VLAN_MEMBER')
                 for key in data.get(table, {}))
    return (not member and row['enabled'] == 'true' and parent.get('enabled') == 'true'
            and routed_vrf(data, name) == row['vrf']
            and (row['ipv4_enabled'] == 'true' or row['ipv6_enabled'] == 'true'))


SNAPSHOT_TABLES = TABLES + ROUTED_TABLES + ('VRF', 'ROUTE_MAP', 'ROUTE_MAP_SET', 'PORT')
SNAPSHOT_TABLES += ('PORTCHANNEL', 'VLAN', 'PORTCHANNEL_MEMBER', 'VLAN_MEMBER')

# One Redis evaluation provides a coherent graph. The subprocess used by the
# runtime bounds a stalled SDK/server; the CLI uses its normal connection.
CAPTURE_LUA = '''
local rows = {}
for _, name in ipairs(ARGV) do
    for _, key in ipairs(redis.call('KEYS', name .. '|*')) do
        table.insert(rows, {name, string.sub(key, string.len(name) + 2), redis.call('HGETALL', key)})
    end
end
return cjson.encode(rows)
'''
COMMIT_LUA = '''
local expected = cjson.decode(ARGV[1])
local updates = cjson.decode(ARGV[2])
for name, rows in pairs(expected) do
    local keys = redis.call('KEYS', name .. '|*')
    local count = 0
    for _, _ in pairs(rows) do count = count + 1 end
    if #keys ~= count then return redis.error_reply('IS-IS configuration changed; retry') end
    for _, key in ipairs(keys) do
        local row = rows[string.sub(key, string.len(name) + 2)]
        if not row then return redis.error_reply('IS-IS configuration changed; retry') end
        local fields = redis.call('HGETALL', key)
        local length = 0
        for field, _ in pairs(row) do if field ~= 'NULL' then length = length + 1 end end
        local actual_length = 0
        for index = 1, #fields, 2 do
            if fields[index] ~= 'NULL' then actual_length = actual_length + 1 end
        end
        if actual_length ~= length then return redis.error_reply('IS-IS configuration changed; retry') end
        for index = 1, #fields, 2 do
            if fields[index] ~= 'NULL' and row[fields[index]] ~= fields[index + 1] then
                return redis.error_reply('IS-IS configuration changed; retry')
            end
        end
    end
end
for _, update in ipairs(updates) do
    redis.call('DEL', update.key)
    for field, value in pairs(update.row) do redis.call('HSET', update.key, field, value) end
end
return 1
'''


def _eval(db, script, *arguments):
    """Use the connected SDK client, preserving its namespace and credentials.

    Current swsscommon returns a native DBConnector, whereas older Python SDKs
    return redis-py. RedisCommand's argument vector preserves Lua/JSON boundaries
    without interpolating data into a Redis command string.
    """
    client = db.get_redis_client(db.CONFIG_DB)
    if callable(getattr(client, 'eval', None)):
        return client.eval(script, 0, *arguments)
    from swsscommon.swsscommon import RedisCommand, RedisReply
    command = RedisCommand()
    command.format(['EVAL', script, '0', *arguments])
    try:
        reply = RedisReply(client, command)
        return RedisReply.to_string(reply)
    except RuntimeError as error:
        # Native errors include the entire command and candidate ConfigDB graph.
        # Report the conflict without echoing all configuration into CLI output.
        if 'IS-IS configuration changed; retry' in str(error):
            raise ValueError('IS-IS configuration changed; retry') from None
        raise RuntimeError('Atomic ConfigDB command failed') from None


def capture(db):
    import json
    rows = json.loads(_eval(db, CAPTURE_LUA, *SNAPSHOT_TABLES))
    data = {name: {} for name in SNAPSHOT_TABLES}
    for table, key, fields in rows:
        raw = dict(zip(fields[::2], fields[1::2]))
        row = {field[:-1] if field.endswith('@') else field:
               value.split(',') if field.endswith('@') else value
               for field, value in raw.items() if field != 'NULL'}
        parts = key_tuple(key)
        data[table][parts if len(parts) > 1 else parts[0]] = row
    return data


def commit(db, before, updates):
    import json

    updates = tuple(updates)
    after = copy.deepcopy(before)
    for table, key, row in updates:
        if row is None:
            after[table].pop(key, None)
        else:
            after[table][key] = row
    validate_transition(before, after)

    def raw(row):
        return {field + '@' if isinstance(value, list) else field:
                ','.join(value) if isinstance(value, list) else str(value)
                for field, value in row.items()} or {'NULL': 'NULL'}

    expected = {name: {'|'.join(key) if isinstance(key, tuple) else key: raw(value)
                       for key, value in rows.items()} for name, rows in before.items()}
    writes = [{'key': table + '|' + ('|'.join(key) if isinstance(key, tuple) else key),
               'row': raw(row) if row is not None else {}} for table, key, row in updates]
    _eval(db, COMMIT_LUA, json.dumps(expected), json.dumps(writes))
