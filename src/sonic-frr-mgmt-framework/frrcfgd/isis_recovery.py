"""Bounded, identity-aware recovery for one default-VRF IS-IS/SRv6 component.

This module does not choose a production storage mount, run vtysh, generate CLI,
schedule workers or change other protocols' replay. The adapter supplies a
complete native readback, a desired snapshot, and a fail-fast mutation transport.
Native defaults are explicit inputs selected for a supported FRR release.
"""

import copy
from dataclasses import dataclass, replace
from contextlib import contextmanager, nullcontext
import fcntl
import hashlib
import ipaddress
import json
import math
import os
from pathlib import Path
import re
import tempfile
import threading
import time
from typing import Any, Mapping, Optional, Tuple


KINDS = frozenset(
    ('isis-process', 'isis-net', 'isis-binding', 'isis-circuit', 'isis-srv6', 'srv6-source', 'srv6-locator', 'srv6-sid')
)


class RecoveryError(ValueError):
    """Unavailable, incomplete or unsupported recovery evidence."""


def _valid_boot_id(value):
    return (
        isinstance(value, str)
        and re.fullmatch(r'[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}', value) is not None
    )


def read_boot_id():
    boot_id = Path('/proc/sys/kernel/random/boot_id').read_text(encoding='ascii').strip()
    if not _valid_boot_id(boot_id):
        raise RecoveryError('Invalid Linux boot ID')
    return boot_id


class _ClockRollback(RecoveryError):
    def __init__(self, result):
        super().__init__(result.reason)
        self.result = result


class _DesiredUnavailable(RecoveryError):
    def __init__(self, result):
        super().__init__(result.reason)
        self.result = result


@dataclass(frozen=True, order=True)
class Identity:
    kind: str
    key: str

    def __post_init__(self):
        if self.kind not in KINDS or not isinstance(self.key, str) or not self.key:
            raise RecoveryError('Invalid managed identity')
        if len(self.key) > 512 or any(ord(character) < 32 for character in self.key):
            raise RecoveryError('Invalid managed identity key')


class _FrozenDict(dict):
    """Immutable snapshot mapping with an explicit mutable deepcopy escape."""

    def _readonly(self, *args, **kwargs):
        raise TypeError('Native snapshot mappings are immutable')

    __setitem__ = __delitem__ = clear = pop = popitem = setdefault = update = __ior__ = _readonly

    def __deepcopy__(self, memo):
        # Projection/planning works on detached ordinary dictionaries, never on
        # a reader's retained native evidence.
        return copy.deepcopy(dict(self), memo)


def _freeze(value):
    if isinstance(value, dict):
        return _FrozenDict({key: _freeze(item) for key, item in value.items()})
    if isinstance(value, (list, tuple)):
        return tuple(_freeze(item) for item in value)
    return value


@dataclass(frozen=True)
class NativeSnapshot:
    objects: Mapping[Identity, Mapping[str, Any]]
    unsupported: Tuple[str, ...] = ()
    unconfirmed: frozenset = frozenset()

    def __post_init__(self):
        # Never retain a reader's mutable ConfigDB/native snapshot by reference.
        objects = copy.deepcopy(dict(self.objects))
        if any(not isinstance(key, Identity) or not isinstance(value, dict) for key, value in objects.items()):
            raise RecoveryError('Invalid snapshot object')
        unconfirmed = frozenset(self.unconfirmed)
        if not unconfirmed <= objects.keys():
            raise RecoveryError('Unconfirmed identities must be present configuration objects')
        json.dumps(_objects_json(objects), allow_nan=False)
        object.__setattr__(self, 'objects', _freeze(objects))
        object.__setattr__(self, 'unsupported', tuple(self.unsupported))
        object.__setattr__(self, 'unconfirmed', unconfirmed)


def _objects_json(objects):
    return [{'kind': key.kind, 'key': key.key, 'value': objects[key]} for key in sorted(objects)]


@dataclass(frozen=True)
class DesiredState:
    snapshot: NativeSnapshot

    @property
    def digest(self):
        if self.snapshot.unconfirmed:
            raise RecoveryError('Desired snapshot cannot contain observational confirmation flags')
        if self.snapshot.unsupported:
            raise RecoveryError('Desired snapshot contains unsupported configuration')
        value = json.dumps(
            _objects_json(self.snapshot.objects), sort_keys=True, separators=(',', ':'), allow_nan=False
        ).encode()
        return hashlib.sha256(value).hexdigest()


@dataclass(frozen=True)
class Mutation:
    # Opaque adapter payload, never a shell command interpreted by this module.
    payload: Any
    identities: frozenset


@dataclass(frozen=True)
class RecoveryResult:
    status: str
    reason: str
    observed: Optional[NativeSnapshot] = None
    mutation_attempted: bool = False
    digest: str = ''
    deadline: Optional[float] = None
    desired_available: bool = True


@dataclass(frozen=True)
class RecoveryRecord:
    owner: str
    owned: frozenset
    possible: frozenset
    digest: str
    status: str
    deadline: Optional[float]
    last_observed: float
    reason: str = ''
    version: int = 1
    boot_id: Optional[str] = None

    def as_json(self):
        record = dict(
            version=self.version,
            owner=self.owner,
            owned=[dict(kind=item.kind, key=item.key) for item in sorted(self.owned)],
            possible=[dict(kind=item.kind, key=item.key) for item in sorted(self.possible)],
            digest=self.digest,
            status=self.status,
            deadline=self.deadline,
            last_observed=self.last_observed,
            reason=self.reason,
        )
        if self.boot_id is not None:
            record['boot_id'] = self.boot_id
        return record

    @classmethod
    def from_json(cls, data):
        expected = {'version', 'owner', 'owned', 'possible', 'digest', 'status', 'deadline', 'last_observed', 'reason'}
        if (
            not isinstance(data, dict)
            or set(data) - {'boot_id'} != expected
            or type(data['version']) is not int
            or data['version'] != 1
        ):
            raise RecoveryError('Unsupported ownership record schema')
        if 'boot_id' in data and not _valid_boot_id(data['boot_id']):
            raise RecoveryError('Invalid stored boot ID')
        if not isinstance(data['owner'], str) or not data['owner']:
            raise RecoveryError('Invalid ownership scope')
        if data['status'] not in ('pending', 'applied', 'failed'):
            raise RecoveryError('Invalid recovery phase')
        if data['status'] == 'applied' and (data['deadline'] is not None or data['possible']):
            raise RecoveryError('Applied record retains an unfinished attempt')
        if not isinstance(data['digest'], str) or not re.fullmatch('[0-9a-f]{64}', data['digest']):
            raise RecoveryError('Invalid desired digest')
        for field in ('last_observed', 'deadline'):
            value = data[field]
            if value is None and field == 'deadline' and data['status'] == 'applied':
                continue
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise RecoveryError('Invalid persistent deadline')
        if not isinstance(data['reason'], str):
            raise RecoveryError('Invalid recovery reason')
        result = dict(data)
        for field in ('owned', 'possible'):
            if not isinstance(data[field], list):
                raise RecoveryError('Invalid identity set')
            for item in data[field]:
                if not isinstance(item, dict) or set(item) != {'kind', 'key'}:
                    raise RecoveryError('Invalid stored identity')
            identities = frozenset(Identity(**item) for item in data[field])
            if len(identities) != len(data[field]):
                raise RecoveryError('Duplicate stored identity')
            result[field] = identities
        return cls(**result)


def prepare_startup_record(
    previous, desired, owner, now, timeout, *, trusted_preapply=False, retry=False, boot_id=None
):
    """Prepare write-ahead registration for trusted canonical startup only.

    This is NOT an adoption API for a matching live FRR configuration. The
    caller validates one coherent desired snapshot and establishes the trusted
    startup/pre-application context. Under store.guard(), it loads the previous
    record, saves this result, checks status == 'pending', and only then emits
    the managed fragment. Save failure or a failed result forbids emission.
    Deadlines and failure latches belong to the boot that created them. A
    trusted startup may start one new attempt on a different Linux boot or when
    migrating a legacy record without a boot ID. It keeps both identity sets.
    """
    if trusted_preapply is not True:
        raise RecoveryError('Startup registration requires trusted pre-application context')
    if not isinstance(desired, DesiredState) or not isinstance(owner, str) or not owner:
        raise RecoveryError('Invalid startup registration scope')
    if (
        type(retry) is not bool
        or isinstance(now, bool)
        or isinstance(timeout, bool)
        or not isinstance(now, (int, float))
        or not isinstance(timeout, (int, float))
        or not math.isfinite(now)
        or not math.isfinite(timeout)
        or timeout <= 0
        or not math.isfinite(now + timeout)
    ):
        raise RecoveryError('Invalid startup registration deadline')
    if boot_id is not None and not _valid_boot_id(boot_id):
        raise RecoveryError('Invalid Linux boot ID')
    digest = desired.digest
    if previous is not None:
        if not isinstance(previous, RecoveryRecord):
            raise RecoveryError('Invalid previous startup registration')
        previous = RecoveryRecord.from_json(previous.as_json())
        if previous.owner != owner:
            raise RecoveryError('Startup ownership scope mismatch')
        if boot_id is None:
            boot_id = previous.boot_id
        same_boot = previous.boot_id == boot_id
        if same_boot and previous.status == 'pending' and now < previous.last_observed:
            return replace(previous, status='failed', reason='Wall clock moved backwards during pending recovery')
        if same_boot and previous.digest == digest and not retry:
            if previous.status == 'failed':
                return previous
            if previous.status == 'pending':
                if now >= previous.deadline:
                    return replace(previous, status='failed', last_observed=now, reason='Recovery deadline expired')
                return replace(
                    previous,
                    possible=frozenset(previous.owned | previous.possible | set(desired.snapshot.objects)),
                    last_observed=now,
                    reason='Canonical startup fragment pending',
                )
    owned = previous.owned if previous else frozenset()
    possible = previous.possible if previous else frozenset()
    return RecoveryRecord(
        owner,
        owned,
        frozenset(owned | possible | set(desired.snapshot.objects)),
        digest,
        'pending',
        now + timeout,
        now,
        'Canonical startup fragment pending',
        boot_id=boot_id,
    )


class AtomicJSONStore:
    """Injectable prototype store; the caller must prove path durability.

    guard() serializes a whole read/plan/mutate/confirm cycle, including across
    cooperating processes. Replacement is durable only after file AND directory
    fsync succeed. An exception never authorizes a subsequent native mutation.
    """

    def __init__(self, path):
        self.path = Path(path)

    @contextmanager
    def guard(self):
        self.path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        descriptor = os.open(str(self.path) + '.lock', os.O_RDWR | os.O_CREAT, 0o600)
        try:
            # A second worker must reschedule, not wait indefinitely behind a
            # stopped or slow peer before it can even inspect its deadline.
            fcntl.flock(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
            yield
        finally:
            os.close(descriptor)

    def load(self):
        try:
            with self.path.open('rb') as stream:
                value = stream.read(1024 * 1024 + 1)
        except FileNotFoundError:
            return None
        if len(value) > 1024 * 1024:
            raise RecoveryError('Ownership record exceeds prototype limit')
        return RecoveryRecord.from_json(json.loads(value))

    def save(self, record):
        data = record.as_json()
        RecoveryRecord.from_json(data)
        self.path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        descriptor, temporary = tempfile.mkstemp(prefix=self.path.name + '.', dir=self.path.parent)
        try:
            with os.fdopen(descriptor, 'w') as stream:
                json.dump(data, stream, sort_keys=True, separators=(',', ':'), allow_nan=False)
                stream.write('\n')
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path)
            directory = os.open(self.path.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


@dataclass(frozen=True)
class NativeDefaults:
    level: str
    ipv6_multi_topology: bool
    dynamic_hostname: bool
    lsp_mtu: int
    lsp_lifetime: int
    lsp_refresh_interval: int
    passive: bool
    metric: int
    circuit_type: str
    block_len: int
    node_len: int
    func_len: int
    arg_len: int
    srv6_interface: str
    node_msd: Mapping[str, int]

    def __post_init__(self):
        fields = {'max-segs-left', 'max-end-pop', 'max-h-encaps', 'max-end-d'}
        if set(self.node_msd) != fields or any(
            type(value) is not int or not 0 <= value <= 255 for value in self.node_msd.values()
        ):
            raise RecoveryError('Explicit verified node MSD defaults are required')
        object.__setattr__(self, 'node_msd', copy.deepcopy(dict(self.node_msd)))


def normalize_net(value):
    if not re.fullmatch('[0-9a-fA-F.]+', value):
        raise RecoveryError('Invalid native NET')
    value = value.replace('.', '').lower()
    if not 16 <= len(value) <= 40 or len(value) % 2 or not value.endswith('00'):
        raise RecoveryError('Invalid native NET length or NSEL')
    return value


def _identifier(value):
    if not re.fullmatch('[A-Za-z0-9_.:/-]+', value):
        raise RecoveryError('Unsupported native identifier')
    return value


def parse_running_config(text, defaults):
    """Parse complete native running-config; never interpret missing output as empty.

    Unrelated sections/foreign SRv6 objects are retained or ignored, not adopted.
    Unknown content inside a relevant context produces an unsupported snapshot.
    The caller must obtain this text through a successful, bounded native query.
    """
    if not isinstance(defaults, NativeDefaults):
        raise RecoveryError('Explicit verified native defaults are required')
    if not isinstance(text, str) or not text.strip():
        raise RecoveryError('Empty native readback')
    lines = [line.rstrip() for line in text.splitlines() if line.strip()]
    if lines[-1].strip() != 'end' or not any(line.startswith('frr version ') for line in lines):
        raise RecoveryError('Incomplete native readback')
    if any(line.lstrip().startswith('%') or 'Unknown command' in line or '\t' in line for line in lines):
        raise RecoveryError('Native readback contains an error or ambiguous indentation')
    objects, unsupported, stack = {}, [], []
    process_defaults = {
        name: getattr(defaults, name)
        for name in (
            'level',
            'ipv6_multi_topology',
            'dynamic_hostname',
            'lsp_mtu',
            'lsp_lifetime',
            'lsp_refresh_interval',
        )
    }
    circuit_defaults = {name: getattr(defaults, name) for name in ('passive', 'metric', 'circuit_type')}

    def add(identity, value):
        if identity in objects:
            raise RecoveryError('Duplicate native identity: ' + str(identity))
        objects[identity] = value
        return value

    for raw in lines:
        command = raw.strip()
        indent = len(raw) - len(raw.lstrip(' '))
        if command == '!':
            if indent == 0:
                stack = []
            continue
        if command == 'end':
            continue
        if command in ('exit', 'exit-vrf'):
            while stack and stack[-1][0] >= indent:
                stack.pop()
            continue
        while stack and indent <= stack[-1][0]:
            stack.pop()
        if indent == 0:
            if command.startswith('interface '):
                parts = command.split()
                if len(parts) not in (2, 4) or (len(parts) == 4 and parts[2] != 'vrf'):
                    raise RecoveryError('Malformed native interface context')
                name = _identifier(parts[1])
                interface_vrf = _identifier(parts[3]) if len(parts) == 4 else 'default'
                stack = [(0, 'interface', (name, interface_vrf))]
            elif command.startswith('router isis '):
                parts = command.split()
                if len(parts) not in (3, 5) or (len(parts) == 5 and parts[3] != 'vrf'):
                    raise RecoveryError('Malformed native IS-IS process')
                tag = _identifier(parts[2])
                vrf = _identifier(parts[4]) if len(parts) == 5 else 'default'
                add(Identity('isis-process', tag), dict(process_defaults, vrf=vrf))
                stack = [(0, 'isis', tag)]
            elif command == 'segment-routing':
                stack = [(0, 'segment-routing', None)]
            elif command.startswith(('router isis', 'segment-routing ')):
                unsupported.append(command)
                stack = [(0, 'ignored', None)]
            elif command.startswith(('frr version ', 'frr defaults ', 'hostname ', 'log ', 'service ')):
                stack = []
            else:
                stack = [(0, 'ignored', None)]
            continue
        if not stack:
            raise RecoveryError('Orphan native context')
        parent, key = stack[-1][1:]
        parts = command.split()
        if parent == 'ignored':
            continue
        if parent == 'interface':
            key, interface_vrf = key
            binding = re.fullmatch(r'(ip|ipv6) router isis ([A-Za-z0-9_.:/-]+)', command)
            if binding:
                if interface_vrf != 'default':
                    unsupported.append('IS-IS binding in nondefault interface VRF: ' + raw)
                    continue
                af = 'ipv4' if binding[1] == 'ip' else 'ipv6'
                tag = binding[2]
                add(Identity('isis-binding', key + '|' + af + '|' + tag), dict(interface=key, af=af, process=tag))
                objects.setdefault(Identity('isis-circuit', key), dict(circuit_defaults))
            elif command.startswith(('isis ', 'no isis ')):
                circuit = objects.setdefault(Identity('isis-circuit', key), dict(circuit_defaults))
                if command in ('isis passive', 'no isis passive'):
                    circuit['passive'] = command == 'isis passive'
                elif re.fullmatch(r'isis metric [0-9]+', command):
                    circuit['metric'] = int(parts[-1])
                elif command in ('isis network point-to-point', 'no isis network point-to-point'):
                    circuit['circuit_type'] = 'p2p' if command.startswith('isis ') else 'lan'
                else:
                    unsupported.append(raw)
            elif 'router isis' in command:
                unsupported.append(raw)
        elif parent == 'isis':
            process = objects[Identity('isis-process', key)]
            if len(parts) == 2 and parts[0] == 'net':
                net = normalize_net(parts[1])
                add(Identity('isis-net', key + '|' + net), dict(process=key, net=net))
            elif command in ('is-type level-1', 'is-type level-2-only', 'is-type level-1-2'):
                process['level'] = {'level-2-only': 'level-2'}.get(parts[1], parts[1])
            elif command in ('topology ipv6-unicast', 'no topology ipv6-unicast'):
                process['ipv6_multi_topology'] = not command.startswith('no ')
            elif command in ('hostname dynamic', 'no hostname dynamic'):
                process['dynamic_hostname'] = not command.startswith('no ')
            elif len(parts) == 2 and parts[0] in ('lsp-mtu', 'max-lsp-lifetime', 'lsp-refresh-interval'):
                field = {
                    'lsp-mtu': 'lsp_mtu',
                    'max-lsp-lifetime': 'lsp_lifetime',
                    'lsp-refresh-interval': 'lsp_refresh_interval',
                }[parts[0]]
                try:
                    process[field] = int(parts[1])
                    if process[field] < 0:
                        raise ValueError()
                except ValueError as error:
                    raise RecoveryError('Invalid native timer') from error
            elif command == 'segment-routing srv6':
                add(
                    Identity('isis-srv6', key),
                    dict(locator=None, interface=defaults.srv6_interface, node_msd=copy.deepcopy(defaults.node_msd)),
                )
                stack.append((indent, 'isis-srv6', key))
            else:
                unsupported.append(raw)
        elif parent == 'isis-srv6':
            if len(parts) == 2 and parts[0] == 'locator':
                objects[Identity('isis-srv6', key)]['locator'] = _identifier(parts[1])
            elif len(parts) == 2 and parts[0] == 'interface':
                objects[Identity('isis-srv6', key)]['interface'] = _identifier(parts[1])
            elif len(parts) == 3 and parts[:2] == ['no', 'interface']:
                _identifier(parts[2])
                objects[Identity('isis-srv6', key)]['interface'] = defaults.srv6_interface
            elif command == 'node-msd':
                stack.append((indent, 'node-msd', key))
            else:
                unsupported.append(raw)
        elif parent == 'node-msd':
            if len(parts) == 2 and parts[0] in ('max-segs-left', 'max-end-pop', 'max-h-encaps', 'max-end-d'):
                try:
                    value = int(parts[1])
                    if not 0 <= value <= 255:
                        raise ValueError()
                except ValueError as error:
                    raise RecoveryError('Invalid native node MSD') from error
                objects[Identity('isis-srv6', key)]['node_msd'][parts[0]] = value
            else:
                unsupported.append(raw)
        elif parent == 'segment-routing':
            stack.append((indent, 'srv6' if command == 'srv6' else 'ignored', None))
        elif parent == 'srv6':
            if command in ('encapsulation', 'locators', 'static-sids'):
                stack.append((indent, command, None))
            else:
                unsupported.append(raw)
                stack.append((indent, 'ignored', None))
        elif parent == 'encapsulation':
            if len(parts) == 2 and parts[0] == 'source-address':
                add(Identity('srv6-source', 'default'), dict(address=str(ipaddress.IPv6Address(parts[1]))))
            else:
                unsupported.append(raw)
        elif parent == 'locators':
            if len(parts) == 2 and parts[0] == 'locator':
                name = _identifier(parts[1])
                values = {field: getattr(defaults, field) for field in ('block_len', 'node_len', 'func_len', 'arg_len')}
                add(Identity('srv6-locator', name), dict(values, prefix=None, behavior=None, format=None))
                stack.append((indent, 'locator', name))
            else:
                unsupported.append(raw)
        elif parent == 'locator':
            locator = objects[Identity('srv6-locator', key)]
            if parts[0] == 'prefix' and len(parts) >= 2:
                network = ipaddress.IPv6Network(parts[1], strict=False)
                locator['prefix'] = str(network)
                # Native writer omissions follow the CLI's node-length
                # default, not the model's explicit block/node defaults.
                locator['node_len'] = defaults.node_len
                locator['block_len'] = network.prefixlen - defaults.node_len
                locator['func_len'], locator['arg_len'] = defaults.func_len, 0
                options = parts[2:]
                if len(options) % 2:
                    raise RecoveryError('Malformed locator geometry')
                names = {'block-len': 'block_len', 'node-len': 'node_len', 'func-bits': 'func_len'}
                for name, value in zip(options[::2], options[1::2]):
                    if name not in names:
                        unsupported.append(raw)
                    else:
                        locator[names[name]] = int(value)
            elif command == 'behavior usid':
                locator['behavior'] = 'usid'
            elif len(parts) == 2 and parts[0] == 'format':
                locator['format'] = _identifier(parts[1])
                if locator['format'] == 'usid-f3216':
                    # FRR's effective locator behavior follows the USID format
                    # type even when its writer omits the raw behavior flag.
                    locator['behavior'] = 'usid'
                else:
                    unsupported.append(raw)
            else:
                unsupported.append(raw)
        elif parent == 'static-sids':
            sid = re.fullmatch(r'sid (\S+) locator (\S+) behavior (uDT46|uDT4|uDT6|uN|uA)(?:\s+(.*))?', command)
            if sid:
                prefix = str(ipaddress.IPv6Network(sid[1], strict=False))
                options = (sid[4] or '').strip().split()
                value = dict(locator=_identifier(sid[2]), behavior=sid[3], vrf=None, interface=None, adj=None)
                if sid[3] == 'uA' and len(options) in (2, 4) and options[0] == 'interface':
                    value['interface'] = _identifier(options[1])
                    if len(options) == 4:
                        if options[2] != 'nexthop':
                            raise RecoveryError('Unsupported uA adjacency syntax')
                        address = ipaddress.IPv6Address(options[3])
                        value['adj'] = None if address.is_unspecified else str(address)
                elif sid[3] in ('uDT4', 'uDT6', 'uDT46') and len(options) == 2 and options[0] == 'vrf':
                    value['vrf'] = _identifier(options[1])
                elif sid[3] != 'uN' or options:
                    raise RecoveryError('Unsupported native static SID parameters')
                add(Identity('srv6-sid', prefix), value)
            else:
                unsupported.append(raw)
    if any(key.kind == 'srv6-locator' and value['prefix'] is None for key, value in objects.items()):
        raise RecoveryError('Incomplete native locator')
    for identity, value in objects.items():
        if identity.kind == 'srv6-locator':
            if not (
                16 <= value['block_len'] <= 64
                and 0 <= value['node_len'] <= 64
                and 0 <= value['func_len'] <= 64
                and value['arg_len'] == 0
                and value['block_len'] + value['node_len'] == ipaddress.IPv6Network(value['prefix']).prefixlen
                and value['block_len'] + value['node_len'] + value['func_len'] <= 128
            ):
                raise RecoveryError('Unsupported native locator geometry')
    return NativeSnapshot(objects, tuple(unsupported))


def _conflict(native, desired, authorized):
    if native.unsupported:
        return 'Unsupported relevant native context: ' + str(native.unsupported[0])
    targets = set(desired.objects) | set(authorized)
    for identity in native.objects:
        if identity in targets and identity not in authorized:
            return 'Live identity lacks ownership: ' + str(identity)
        if identity.kind == 'isis-process' and any(item.kind == 'isis-process' for item in targets):
            if identity.key != 'default' or native.objects[identity].get('vrf') != 'default':
                return 'Foreign IS-IS process prevents single-instance reconciliation'
        if identity.kind == 'isis-net' and identity not in authorized:
            if any(item.kind == 'isis-process' and item.key == native.objects[identity]['process'] for item in targets):
                return 'Unowned NET in affected IS-IS process'
        if identity.kind == 'isis-binding':
            process = Identity('isis-process', native.objects[identity].get('process', '<absent>'))
            if process.key != 'default' and Identity('isis-process', 'default') in targets:
                return 'Foreign IS-IS binding prevents single-instance reconciliation'
            if (
                identity not in authorized
                and process in targets
                and native.objects.get(process) != desired.objects.get(process)
            ):
                return 'Unowned native binding prevents process mutation'
    for identity, value in native.objects.items():
        if identity in authorized or identity.kind not in ('srv6-sid', 'isis-srv6'):
            continue
        if identity.kind == 'isis-srv6':
            process = Identity('isis-process', identity.key)
            if process in targets and native.objects.get(process) != desired.objects.get(process):
                return 'Unowned native SRv6 child prevents process mutation'
        locator = Identity('srv6-locator', value.get('locator') or '<absent>')
        if locator in targets and native.objects.get(locator) != desired.objects.get(locator):
            return 'Unowned native reference prevents locator mutation'
    return None


def _matches(native, desired, authorized):
    scope = set(authorized) | set(desired.objects)
    if native.unconfirmed & scope:
        return False
    for identity in scope:
        if native.objects.get(identity) != desired.objects.get(identity):
            return False
    return True


class RecoveryKernel:
    """One bounded component step. Store/transport errors never mean absence.

    readback() returns NativeSnapshot from a successful complete native query.
    planner(native, desired.snapshot, authorized) returns Mutation or None.
    apply(mutation.payload) uses a bounded, fail-fast adapter; its boolean is
    merely a response hint. Only subsequent native equality confirms success.
    A new desired digest or explicit retry starts a new absolute deadline.
    """

    def __init__(self, store, owner='default', clock=time.time, *, boot_id_supplier=None):
        self.store, self.owner, self.clock = store, owner, clock
        self.boot_id_supplier = boot_id_supplier
        self._lock = threading.Lock()

    def step(self, desired_supplier, readback, planner, apply, timeout, retry=False):
        if isinstance(timeout, bool) or not math.isfinite(timeout) or timeout <= 0:
            raise RecoveryError('Recovery timeout must be positive and finite')
        with self._lock:
            guard = getattr(self.store, 'guard', nullcontext)
            try:
                with guard():
                    return self._step(desired_supplier, readback, planner, apply, timeout, retry)
            except _ClockRollback as error:
                return error.result
            except _DesiredUnavailable as error:
                return error.result
            except (OSError, ValueError, TypeError) as error:
                return RecoveryResult('metadata_error', str(error))

    def _step(self, desired_supplier, readback, planner, apply, timeout, retry):
        record = self.store.load()
        now = float(self.clock())
        boot_id = record.boot_id if record is not None else None
        if self.boot_id_supplier is not None:
            boot_id = self.boot_id_supplier()
        attempted = False
        observed_hint = None
        if not math.isfinite(now):
            raise RecoveryError('Invalid recovery clock')
        if record is not None:
            if record.owner != self.owner:
                return RecoveryResult('conflict', 'Ownership scope mismatch', digest=record.digest)

        def expire_pending(checked_at):
            nonlocal record
            if not math.isfinite(checked_at):
                raise RecoveryError('Invalid recovery clock')
            if record is not None and record.status == 'pending':
                if checked_at < record.last_observed:
                    record = replace(
                        record, status='failed', reason='Wall clock moved backwards during pending recovery'
                    )
                    self.store.save(record)
                elif checked_at >= record.deadline:
                    record = replace(
                        record, status='failed', last_observed=checked_at, reason='Recovery deadline expired'
                    )
                    self.store.save(record)

        def unavailable(error):
            nonlocal record
            checked_at = float(self.clock())
            expire_pending(checked_at)
            if record is not None and record.status == 'pending':
                record = replace(record, last_observed=checked_at, reason='Desired configuration unavailable')
                self.store.save(record)
            failed = record is not None and record.status == 'failed'
            result = RecoveryResult(
                'failed' if failed else 'desired_error',
                (record.reason or 'Previous recovery attempt failed') if failed else str(error),
                observed=observed_hint,
                mutation_attempted=attempted,
                digest=record.digest if record else '',
                deadline=record.deadline if record else None,
                desired_available=False,
            )
            raise _DesiredUnavailable(result)

        def desired_digest():
            try:
                return desired_supplier().digest
            except Exception as error:
                unavailable(error)

        # An unavailable renderer/factory cannot hide an expired persisted
        # attempt, and must not consume the caller's explicit retry request.
        expire_pending(now)
        try:
            desired = desired_supplier()
            digest = desired.digest
        except Exception as error:
            unavailable(error)
        if record is None or record.digest != digest or retry:
            record = RecoveryRecord(
                self.owner,
                record.owned if record else frozenset(),
                record.possible if record else frozenset(),
                digest,
                'pending',
                now + timeout,
                now,
                boot_id=boot_id,
            )
            self.store.save(record)
        elif record.status == 'failed':
            return RecoveryResult('failed', record.reason, digest=digest, deadline=record.deadline)

        def checked_now():
            nonlocal record
            value = float(self.clock())
            if not math.isfinite(value):
                raise RecoveryError('Invalid recovery clock')
            if record.status == 'pending' and value < record.last_observed:
                record = replace(record, status='failed', reason='Wall clock moved backwards during pending recovery')
                self.store.save(record)
                raise _ClockRollback(
                    RecoveryResult(
                        'failed', record.reason, mutation_attempted=attempted, digest=digest, deadline=record.deadline
                    )
                )
            return value

        def begin_attempt():
            nonlocal record
            if record.status == 'applied':
                record = replace(
                    record, status='pending', deadline=now + timeout, last_observed=now, reason='', boot_id=boot_id
                )

        def pending(status, reason, observed=None, attempted=False):
            nonlocal record
            begin_attempt()
            checked_at = checked_now()
            if checked_at >= record.deadline:
                status, reason = 'failed', 'Recovery deadline expired'
                record = replace(record, status='failed')
            record = replace(record, reason=reason, last_observed=checked_at)
            try:
                self.store.save(record)
            except (OSError, ValueError, TypeError) as error:
                return RecoveryResult('metadata_error', str(error), observed, attempted, digest, record.deadline)
            return RecoveryResult(status, reason, observed, attempted, digest, record.deadline)

        def observe():
            value = readback()
            if not isinstance(value, NativeSnapshot):
                raise RecoveryError('Reader did not return a complete typed native snapshot')
            return value

        try:
            native = observe()
            observed_hint = native
        except Exception as error:
            return pending('readback_error', str(error))
        if desired_digest() != digest:
            return pending('stale', 'Desired changed during native snapshot', native)
        authorized = record.owned | record.possible
        conflict = _conflict(native, desired.snapshot, authorized)
        if conflict:
            return pending('conflict', conflict, native)

        def confirm(observed, attempted):
            nonlocal record
            if desired_digest() != digest:
                return pending('stale', 'Desired changed before confirmation', observed, attempted)
            if record.deadline is not None and checked_now() >= record.deadline:
                record = replace(record, status='failed', reason='Recovery deadline expired')
                try:
                    self.store.save(record)
                except (OSError, ValueError, TypeError) as error:
                    return RecoveryResult('metadata_error', str(error), observed, attempted, digest, record.deadline)
                return RecoveryResult('failed', record.reason, observed, attempted, digest, record.deadline)
            if (
                record.status == 'applied'
                and record.owned == frozenset(desired.snapshot.objects)
                and not record.possible
            ):
                return RecoveryResult('applied', 'Native state confirmed', observed, attempted, digest)
            final = replace(
                record,
                owned=frozenset(desired.snapshot.objects),
                possible=frozenset(),
                status='applied',
                deadline=None,
                reason='',
                last_observed=checked_now(),
            )
            try:
                self.store.save(final)
            except (OSError, ValueError, TypeError) as error:
                return RecoveryResult('metadata_error', str(error), observed, attempted, digest, record.deadline)
            return RecoveryResult('applied', 'Native state confirmed', observed, attempted, digest)

        if _matches(native, desired.snapshot, authorized):
            return confirm(native, False)
        if record.status == 'applied':
            begin_attempt()
            self.store.save(record)
        try:
            mutation = planner(native, desired.snapshot, frozenset(authorized | set(desired.snapshot.objects)))
            if not isinstance(mutation, Mutation) or not mutation.identities:
                return pending('pending', 'Planner supplied no bounded delta', native)
            if not mutation.identities <= (authorized | set(desired.snapshot.objects)):
                return pending('conflict', 'Planner attempted an unowned identity', native)
        except Exception as error:
            return pending('plan_error', str(error), native)
        record = replace(record, possible=frozenset(authorized | set(desired.snapshot.objects) | mutation.identities))
        # The recoverable old+possible union must be durable before any command.
        self.store.save(record)
        if desired_digest() != digest:
            return pending('stale', 'Desired changed before mutation', native)
        if checked_now() >= record.deadline:
            return pending('failed', 'Recovery deadline expired before mutation', native)
        response = True
        attempted = True
        try:
            response = bool(apply(mutation.payload))
        except Exception:
            response = False
        try:
            observed = observe()
            observed_hint = observed
        except Exception as error:
            return pending('readback_error', str(error), attempted=True)
        if desired_digest() != digest:
            return pending('stale', 'Desired changed during mutation', observed, True)
        conflict = _conflict(observed, desired.snapshot, record.owned | record.possible)
        if conflict:
            return pending('conflict', conflict, observed, True)
        if _matches(observed, desired.snapshot, record.owned | record.possible):
            return confirm(observed, True)
        reason = 'Native delta remains' if response else 'Uncertain mutation response; native delta remains'
        return pending('pending', reason, observed, True)
