"""Register trusted cfggen startup intent before any managed native apply.

The caller freezes one ConfigDB read as a mode-0600 snapshot, renders the
candidate FRR file from those exact JSON bytes, then invokes this module before
starting FRR. Full and fallback renders never read ConfigDB again.
It owns cleanup of the snapshot and fallback rendering; this module never starts
FRR, applies commands, rewrites a candidate, or marks a record as applied.
"""

import argparse
from dataclasses import dataclass, fields
import json
import math
import os
from pathlib import Path
import re
import stat
import time

from .isis_config import validate_isis_config, validate_srv6_config
from .isis_desired import normalize_version, parse_canonical_config, verified_defaults
from .isis_recovery import (
    AtomicJSONStore, DesiredState, NativeDefaults, RecoveryError,
    prepare_startup_record, read_boot_id,
)


@dataclass(frozen=True)
class StartupOutcome:
    status: str
    reason: str
    digest: str = ''
    profile: str = 'canonical'

    def as_json(self):
        return dict(status=self.status, reason=self.reason, digest=self.digest, profile=self.profile)


def _read_text(path, limit, private=False):
    with Path(path).open('r', encoding='utf-8') as stream:
        metadata = stat.S_IMODE(os.fstat(stream.fileno()).st_mode)
        if private and metadata != 0o600:
            raise RecoveryError('Startup snapshot must have mode 0600')
        value = stream.read(limit + 1)
    if len(value) > limit:
        raise RecoveryError('Startup input exceeds size limit')
    return value


def load_native_defaults(path):
    """Load an explicit, version-selected defaults envelope without fallbacks."""
    envelope = json.loads(_read_text(path, 1024 * 1024))
    if (not isinstance(envelope, dict) or set(envelope) != {'frr_version', 'defaults'} or
            not isinstance(envelope['frr_version'], str) or
            not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', envelope['frr_version'])):
        raise RecoveryError('Invalid native defaults envelope')
    values = envelope['defaults']
    if not isinstance(values, dict) or set(values) != {field.name for field in fields(NativeDefaults)}:
        raise RecoveryError('Incomplete native defaults')
    for name in ('ipv6_multi_topology', 'dynamic_hostname', 'passive'):
        if type(values[name]) is not bool:
            raise RecoveryError('Invalid native boolean default')
    for name, lower, upper in (
            ('lsp_mtu', 128, 4352), ('lsp_lifetime', 350, 65535),
            ('lsp_refresh_interval', 1, 65235), ('metric', 1, 16777215),
            ('block_len', 16, 64), ('node_len', 0, 64), ('func_len', 0, 64), ('arg_len', 0, 128)):
        if type(values[name]) is not int or not lower <= values[name] <= upper:
            raise RecoveryError('Invalid native numeric default')
    if (values['level'] not in ('level-1', 'level-2', 'level-1-2') or
            values['circuit_type'] not in ('lan', 'p2p') or
            not isinstance(values['srv6_interface'], str) or
            not re.fullmatch(r'[A-Za-z0-9_.:/-]+', values['srv6_interface']) or
            sum(values[name] for name in ('block_len', 'node_len', 'func_len', 'arg_len')) > 128 or
            values['lsp_lifetime'] < values['lsp_refresh_interval'] + 300):
        raise RecoveryError('Invalid native defaults contract')
    return NativeDefaults(**values), envelope['frr_version']


def prepare_startup(snapshot_path, rendered_path, native_defaults_path=None, *, frr_version=None,
                    store=None, owner='default', timeout=30, clock=time.time, namespace=None):
    """Return prepared only after a durable pending write-ahead registration.

    Input paths are trusted caller-owned products of one frozen cfggen snapshot.
    No arbitrary live state is adopted. Errors return a fallback outcome containing
    a stage name, never configuration contents or credentials from input files.
    """
    stage = 'input'
    namespace = os.environ.get('NAMESPACE_ID', '') if namespace is None else namespace
    profile = 'legacy' if namespace else 'canonical'
    try:
        config = json.loads(_read_text(snapshot_path, 16 * 1024 * 1024, private=True))
        if not isinstance(config, dict):
            raise RecoveryError('Invalid startup snapshot')
        globals_ = config.get('ISIS_GLOBALS', {})
        if not isinstance(globals_, dict) or any(not isinstance(row, dict) for row in globals_.values()):
            raise RecoveryError('Invalid IS-IS global snapshot')
        if namespace:
            return StartupOutcome('fallback', 'Existing namespace startup path excludes canonical SRv6',
                                  profile='legacy')
        if any(key != 'default' for key in globals_):
            profile = 'legacy'
            if any('srv6_locator' in row or 'srv6_interface' in row for row in globals_.values()):
                return StartupOutcome('fallback', 'Legacy IS-IS profile cannot bind canonical SRv6',
                                      profile=profile)
            return StartupOutcome('legacy', 'Existing named IS-IS startup path', profile=profile)
        if (native_defaults_path is None) == (frr_version is None):
            raise RecoveryError('Select exactly one native defaults source')
        if native_defaults_path is not None:
            defaults, version = load_native_defaults(native_defaults_path)
        else:
            version = '.'.join(str(part) for part in normalize_version(frr_version))
            defaults = verified_defaults(normalize_version(version))
        stage = 'validation'
        validate_isis_config(config)
        validate_srv6_config(config, frr_version=normalize_version(version))
        stage = 'rendered configuration'
        rendered = _read_text(rendered_path, 16 * 1024 * 1024)
        desired = DesiredState(parse_canonical_config(rendered, defaults, version))
        stage = 'journal'
        if owner != 'default':
            raise RecoveryError('Unsupported startup ownership scope')
        if isinstance(timeout, bool) or not isinstance(timeout, (int, float)) or not math.isfinite(timeout):
            raise RecoveryError('Invalid startup timeout')
        boot_id = read_boot_id()
        store = store if store is not None else AtomicJSONStore('/etc/frr/frrcfgd-owned.json')
        with store.guard():
            record = prepare_startup_record(store.load(), desired, owner, clock(), timeout,
                                            trusted_preapply=True, boot_id=boot_id)
            store.save(record)
            if record.status != 'pending':
                return StartupOutcome('fallback', 'Startup registration remains failed')
        return StartupOutcome('prepared', 'Pending startup registration saved', desired.digest)
    except (OSError, ValueError, TypeError, KeyError, AttributeError):
        return StartupOutcome('fallback', 'Startup ' + stage + ' failed', profile=profile)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True)
    parser.add_argument('--rendered-config', required=True)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument('--native-defaults')
    selection.add_argument('--frr-version')
    parser.add_argument('--journal', default='/etc/frr/frrcfgd-owned.json')
    parser.add_argument('--owner', default='default')
    parser.add_argument('--namespace', default=os.environ.get('NAMESPACE_ID', ''))
    parser.add_argument('--timeout', type=float, default=30)
    options = parser.parse_args(argv)
    outcome = prepare_startup(options.snapshot, options.rendered_config, options.native_defaults,
                              frr_version=options.frr_version, store=AtomicJSONStore(options.journal),
                              owner=options.owner,
                              timeout=options.timeout, namespace=options.namespace)
    print(json.dumps(outcome.as_json(), sort_keys=True))
    return 0 if outcome.status in ('prepared', 'legacy') else 1


if __name__ == '__main__':
    raise SystemExit(main())
