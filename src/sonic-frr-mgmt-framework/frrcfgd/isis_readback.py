"""Strict readback of the IS-IS subset supported by the pinned FRR model."""
import json
import re
from sonic_isis import LEVELS, BACKOFF, effective, identifier, net
from .isis_config import process_key
from .isis_recovery import Identity, NativeSnapshot, RecoveryError


LEVEL_COMMANDS = ('metric', 'hello-interval', 'hello-multiplier')


def interface_vrfs(text):
    """Read actual interface ownership from Zebra's all-VRF JSON inventory."""
    try:
        rows = json.loads(text)
        if not isinstance(rows, dict):
            raise ValueError('Interface inventory is not an object')
        return {identifier(name): identifier(row['vrfName']) for name, row in rows.items()}
    except (ValueError, TypeError, KeyError) as error:
        raise RecoveryError('Invalid native interface VRF inventory') from error


def check_system_ids(snapshot, text):
    """A configured NET is insufficient if FRR lost its shared runtime ID."""
    try:
        rows = json.loads(text)['vrfs']
        if not isinstance(rows, list):
            raise ValueError('VRF inventory is not an array')
        ids = {}
        for row in rows:
            vrf = identifier(row['vrf'])
            if vrf in ids:
                raise ValueError('Duplicate VRF summary')
            ids[vrf] = row.get('system-id')
    except (ValueError, TypeError, KeyError) as error:
        raise RecoveryError('Invalid native IS-IS system-ID summary') from error
    for identity, value in snapshot.objects.items():
        if identity.kind != 'isis-process' or not value.get('net_titles'):
            continue
        vrf = identity.key.split('|')[0]
        expected = value['net_titles'][0].replace('.', '')[-14:-2]
        actual = ids.get(vrf)
        if (not isinstance(actual, str) or not re.fullmatch(r'[0-9a-fA-F]{4}(\.[0-9a-fA-F]{4}){2}', actual)
                or actual.replace('.', '').lower() != expected):
            raise RecoveryError('Native IS-IS system ID missing or inconsistent in VRF ' + vrf)


def parse(text, observed_vrfs=None):
    if not isinstance(text, str) or not text.strip().endswith('end') or 'frr version ' not in text:
        raise RecoveryError('Incomplete native running configuration')
    objects, circuits, unsupported, declared_vrfs = {}, {}, [], {}
    context, value, key = None, None, None
    for raw in text.splitlines():
        command = raw.strip()
        if not command or command == '!':
            continue
        if command in ('end', 'exit', 'exit-vrf'):
            context, value = None, None
            continue
        tokens = command.split()
        if command.startswith('router isis '):
            if len(tokens) not in (3, 5) or (len(tokens) == 5 and tokens[3] != 'vrf'):
                raise RecoveryError('Invalid native IS-IS router header')
            tag = identifier(tokens[2])
            vrf = identifier(tokens[4]) if len(tokens) == 5 else 'default'
            key = Identity('isis-process', process_key(vrf, tag))
            if key in objects:
                raise RecoveryError('Duplicate native IS-IS instance')
            value = effective('ISIS_INSTANCE', {})
            value.pop('enabled')
            value.update(net_titles=[], redistribute={}, levels={level: effective('ISIS_INSTANCE_LEVEL', {})
                                                                 for level in LEVELS})
            objects[key] = value
            context = 'process'
            continue
        if command.startswith('interface '):
            if len(tokens) not in (2, 4) or (len(tokens) == 4 and tokens[2] != 'vrf'):
                raise RecoveryError('Invalid native interface header')
            name = identifier(tokens[1])
            if name in circuits:
                raise RecoveryError('Duplicate native interface context')
            vrf = identifier(tokens[3]) if len(tokens) == 4 else 'default'
            if len(tokens) == 4:
                declared_vrfs[name] = vrf
            value = effective('ISIS_INTERFACE', {'vrf': vrf})
            value.pop('enabled')
            value['levels'] = {level: effective('ISIS_INTERFACE_LEVEL', {}) for level in LEVELS}
            circuits[name] = value
            key, context = name, 'interface'
            continue
        if not raw.startswith((' ', '\t')):
            context, value = None, None
            continue
        if context == 'process':
            scalar = {'is-type': 'level_type', 'metric-style': 'metric_style', 'lsp-mtu': 'lsp_mtu'}
            if tokens[0] == 'net' and len(tokens) == 2:
                value['net_titles'].append(net(tokens[1]))
            elif tokens[0] in scalar and len(tokens) == 2:
                value[scalar[tokens[0]]] = tokens[1]
            elif command in ('hostname dynamic', 'no hostname dynamic'):
                value['dynamic_hostname'] = 'false' if tokens[0] == 'no' else 'true'
            elif command in ('topology ipv6-unicast', 'no topology ipv6-unicast'):
                value['ipv6_multi_topology'] = 'false' if tokens[0] == 'no' else 'true'
            elif command in ('set-overload-bit', 'no set-overload-bit'):
                value['overload'] = 'false' if tokens[0] == 'no' else 'true'
            elif tokens[:2] == ['set-overload-bit', 'on-startup'] and len(tokens) == 3:
                value['overload_on_startup'] = tokens[2]
            elif tokens[:3] == ['no', 'set-overload-bit', 'on-startup']:
                value['overload_on_startup'] = '0'
            elif tokens[0] == 'lsp-timers':
                rest = tokens[1:]
                levels = [rest.pop(0)] if rest and rest[0] in LEVELS else LEVELS
                if len(rest) != 6 or rest[::2] != ['gen-interval', 'refresh-interval', 'max-lifetime']:
                    raise RecoveryError('Invalid native LSP timers')
                for level in levels:
                    value['levels'][level].update(zip(('lsp_generation_interval', 'lsp_refresh_interval',
                                                       'lsp_maximum_lifetime'), rest[1::2]))
            elif tokens[0] in ('lsp-gen-interval', 'lsp-refresh-interval', 'max-lsp-lifetime', 'spf-interval'):
                field = {'lsp-gen-interval': 'lsp_generation_interval', 'lsp-refresh-interval': 'lsp_refresh_interval',
                         'max-lsp-lifetime': 'lsp_maximum_lifetime', 'spf-interval': 'spf_minimum_interval'}[tokens[0]]
                levels = [tokens[1]] if len(tokens) == 3 and tokens[1] in LEVELS else LEVELS
                if len(tokens) not in (2, 3):
                    raise RecoveryError('Invalid native timer')
                for level in levels:
                    value['levels'][level][field] = tokens[-1]
            elif tokens[0] == 'spf-delay-ietf':
                if len(tokens) != 11 or tokens[1::2] != ['init-delay', 'short-delay', 'long-delay',
                                                         'holddown', 'time-to-learn']:
                    raise RecoveryError('Incomplete native SPF backoff')
                value.update(zip(BACKOFF, tokens[2::2]))
            elif command == 'no spf-delay-ietf':
                for field in BACKOFF:
                    value.pop(field, None)
            elif tokens[0] == 'redistribute' and len(tokens) >= 4:
                policy = '|'.join(tokens[1:4])
                attributes = effective('ISIS_REDISTRIBUTE', {})
                rest = tokens[4:]
                if len(rest) % 2:
                    raise RecoveryError('Invalid native redistribution')
                for field, item in zip(rest[::2], rest[1::2]):
                    if field not in ('metric', 'route-map'):
                        raise RecoveryError('Unsupported native redistribution')
                    attributes[field.replace('-', '_')] = item
                value['redistribute'][policy] = attributes
            else:
                unsupported.append('{}: {}'.format(key.key, command))
        elif context == 'interface':
            match = re.fullmatch(r'(ip|ipv6) router isis ([A-Za-z0-9_.-]+)', command)
            if match:
                if value.get('instance') not in (None, match[2]):
                    raise RecoveryError('Interface has conflicting native area bindings')
                value['instance'] = match[2]
                value['process'] = process_key(value['vrf'], match[2])
                value['ipv4_enabled' if match[1] == 'ip' else 'ipv6_enabled'] = 'true'
            elif command.startswith(('no ip router isis ', 'no ipv6 router isis ')):
                value['ipv4_enabled' if tokens[1] == 'ip' else 'ipv6_enabled'] = 'false'
            elif tokens[:2] == ['isis', 'circuit-type'] and len(tokens) == 3:
                value['level_type'] = tokens[2]
            elif command in ('isis passive', 'no isis passive'):
                value['passive'] = 'false' if tokens[0] == 'no' else 'true'
            elif command in ('isis network point-to-point', 'no isis network point-to-point'):
                value['network_type'] = 'broadcast' if tokens[0] == 'no' else 'point-to-point'
            elif len(tokens) in (3, 4) and tokens[0] == 'isis' and tokens[1] in LEVEL_COMMANDS:
                field = tokens[1].replace('-', '_')
                levels = [tokens[2]] if len(tokens) == 4 and tokens[2] in LEVELS else LEVELS
                for level in levels:
                    value['levels'][level][field] = tokens[-1]
            elif command.startswith(('isis ', 'no isis ', 'ip router isis', 'ipv6 router isis')):
                unsupported.append('{}: {}'.format(key, command))
    for name, value in circuits.items():
        if 'instance' in value and (value['ipv4_enabled'] == 'true' or value['ipv6_enabled'] == 'true'):
            # The pinned isisd writer omits interface VRF qualifiers, including
            # for nondefault VRFs. Never infer actual ownership from ConfigDB or
            # an area tag, which may be reused in several VRFs.
            if observed_vrfs is not None:
                if name not in observed_vrfs:
                    raise RecoveryError('Native IS-IS interface absent from Zebra inventory: ' + name)
                vrf = observed_vrfs[name]
                if name in declared_vrfs and declared_vrfs[name] != vrf:
                    raise RecoveryError('Native interface VRF observations disagree: ' + name)
                value['vrf'] = vrf
                value['process'] = process_key(vrf, value['instance'])
            objects[Identity('isis-binding', name)] = value
    for identity, value in objects.items():
        if identity.kind == 'isis-process':
            value['net_titles'].sort()
    return NativeSnapshot(objects, tuple(unsupported))
