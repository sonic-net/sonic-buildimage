"""Project the shared northbound model and render pinned FRR commands.

AF controls retain gs1571's independent binding semantics. Both startup and the
recovery delta planner use these command builders, including concrete L1/L2 values.
"""
from sonic_isis import (LEVELS, BACKOFF, effective, validate, active_interface)
from .isis_recovery import Identity, NativeSnapshot, DesiredState


def process_key(vrf, instance):
    return vrf + '|' + instance


def router(key):
    vrf, instance = key.split('|')
    return 'router isis ' + instance + ((' vrf ' + vrf) if vrf != 'default' else '')


def interface(name, vrf):
    return 'interface ' + name + ((' vrf ' + vrf) if vrf != 'default' else '')


def desired(tables):
    data = validate(tables)
    objects = {}
    for key, row in sorted(data['ISIS_INSTANCE'].items()):
        if row.get('enabled') != 'true':
            continue
        value = effective('ISIS_INSTANCE', row)
        value.pop('enabled')
        value['net_titles'] = sorted(value.get('net_titles', []))
        value['levels'] = {level: effective('ISIS_INSTANCE_LEVEL', data['ISIS_INSTANCE_LEVEL'].get(key + (level,), {}))
                           for level in LEVELS}
        value['redistribute'] = {'|'.join(k[2:]): effective('ISIS_REDISTRIBUTE', v)
                                 for k, v in data['ISIS_REDISTRIBUTE'].items() if k[:2] == key}
        objects[Identity('isis-process', process_key(*key))] = value
    for name, row in sorted(data['ISIS_INTERFACE'].items()):
        if not active_interface(data, name):
            continue
        value = effective('ISIS_INTERFACE', row)
        value.pop('enabled')
        value['process'] = process_key(value['vrf'], value['instance'])
        value['levels'] = {level: effective('ISIS_INTERFACE_LEVEL', data['ISIS_INTERFACE_LEVEL'].get((name, level), {}))
                           for level in LEVELS}
        objects[Identity('isis-binding', name)] = value
    return DesiredState(NativeSnapshot(objects))


def process_commands(key, value, old=None):
    old = old or {}
    commands = [router(key)]
    # Withdraw policies before reducing the instance's supported levels.
    for policy in sorted(set(old.get('redistribute', {})) - set(value['redistribute'])):
        commands.append('no redistribute ' + policy.replace('|', ' '))
    previous, following = set(old.get('net_titles', [])), set(value['net_titles'])
    if previous and following and not previous & following:
        # Do not temporarily remove the final NET: pinned FRR clears the shared
        # system ID even when sibling areas still have NETs. Keep one old NET
        # until the first new one is installed, respecting the three-NET limit.
        anchor = sorted(previous)[0]
        commands += ['no net ' + address for address in sorted(previous - {anchor})]
        commands += ['net ' + sorted(following)[0], 'no net ' + anchor]
    else:
        commands += ['no net ' + address for address in sorted(previous - following)]
    commands += ['net ' + address for address in value['net_titles']]
    commands += ['is-type ' + value['level_type'], 'metric-style ' + value['metric_style'],
                 'lsp-mtu ' + value['lsp_mtu'],
                 ('' if value['dynamic_hostname'] == 'true' else 'no ') + 'hostname dynamic',
                 ('' if value['ipv6_multi_topology'] == 'true' else 'no ') + 'topology ipv6-unicast',
                 ('' if value['overload'] == 'true' else 'no ') + 'set-overload-bit',
                 'set-overload-bit on-startup ' + value['overload_on_startup']]
    for level in LEVELS:
        fields = value['levels'][level]
        # FRR offers an atomic YANG transaction for the coupled LSP timers.
        commands.append('lsp-timers {} gen-interval {} refresh-interval {} max-lifetime {}'.format(
            level, fields['lsp_generation_interval'], fields['lsp_refresh_interval'], fields['lsp_maximum_lifetime']))
        commands.append('spf-interval {} {}'.format(level, fields['spf_minimum_interval']))
    if BACKOFF[0] in value:
        commands.append('spf-delay-ietf init-delay {} short-delay {} long-delay {} holddown {} time-to-learn {}'.format(
            *(value[field] for field in BACKOFF)))
    else:
        commands.append('no spf-delay-ietf')
    for policy, fields in sorted(value['redistribute'].items()):
        command = 'redistribute ' + policy.replace('|', ' ') + ' metric ' + fields['metric']
        if fields.get('route_map'):
            command += ' route-map ' + fields['route_map']
        commands.append(command)
    return commands + ['exit']


def binding_commands(name, value):
    commands = [interface(name, value['vrf'])]
    for af, prefix in (('ipv4', 'ip'), ('ipv6', 'ipv6')):
        commands.append(('' if value[af + '_enabled'] == 'true' else 'no ') +
                        prefix + ' router isis ' + value['instance'])
    commands += ['isis circuit-type ' + value['level_type'],
                 ('' if value['passive'] == 'true' else 'no ') + 'isis passive',
                 ('' if value['network_type'] == 'point-to-point' else 'no ') + 'isis network point-to-point']
    for level in LEVELS:
        fields = value['levels'][level]
        for field, command in (('metric', 'metric'), ('hello_interval', 'hello-interval'),
                               ('hello_multiplier', 'hello-multiplier')):
            commands.append('isis {} {} {}'.format(command, level, fields[field]))
    return commands + ['exit']


def render(tables):
    snapshot = desired(tables).snapshot
    lines = []
    # Parents must exist before any circuits, irrespective of identity sort order.
    for kind, builder in (('isis-process', process_commands), ('isis-binding', binding_commands)):
        for identity, value in sorted(snapshot.objects.items()):
            if identity.kind == kind:
                block = builder(identity.key, value)
                lines += [block[0]] + [' ' + line for line in block[1:-1]] + ['!']
    return lines
