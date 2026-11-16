"""Projection of trusted cfggen output for the single-instance adapter.

This is not a native readback/adoption API. Native replies must go directly to
the strict parser; only canonical startup/runtime renders may reopen the default
IS-IS process in separate templates.
"""

import copy
from dataclasses import replace
import re

from .isis_recovery import NativeDefaults, NativeSnapshot, RecoveryError, parse_running_config


def normalize_version(version):
    """Normalize an explicit FRR release supplied by the installed binary."""
    if isinstance(version, str) and re.fullmatch(r'[0-9]+\.[0-9]+(?:\.[0-9]+)?', version):
        version = tuple(int(part) for part in version.split('.'))
    if (not isinstance(version, tuple) or len(version) not in (2, 3) or
            any(type(part) is not int or part < 0 for part in version)):
        raise RecoveryError('Explicit target FRR version is required')
    return version


def verified_defaults(version):
    """Return defaults checked in the supported FRR release sources.

    Unknown releases fail the managed slice instead of inventing defaults or
    affecting the caller's independent fallback for other routing protocols.
    Source compatibility does not replace package/native release validation.
    """
    version = normalize_version(version)
    if version[:2] not in ((10, 5), (10, 7)):
        raise RecoveryError('Native defaults are not verified for this FRR release')
    # frr-10.5.4 and SONiC's 10.7.1 gitlink f8c0b08dcb0c retain these defaults:
    # yang/frr-isisd.yang, isisd/isis_srv6.{c,h}, zebra/zebra_srv6_vty.{c,h}.
    # Locator writer omission derives block length from prefixlen - node_len;
    # these native defaults differ from explicit ConfigDB geometry 32/16/16/0.
    return NativeDefaults('level-1-2', False, True, 1497, 1200, 900,
                          False, 10, 'lan', 40, 24, 16, 0, 'sr0',
                          {'max-segs-left': 3, 'max-end-pop': 3,
                           'max-h-encaps': 2, 'max-end-d': 5})


def project_managed_snapshot(snapshot):
    """Copy managed comparison state without taking ownership of node MSD.

    Keep the raw native snapshot separately for guards on removal of a parent.
    Circuits without an active IS-IS AF binding do not represent desired state.
    """
    if not isinstance(snapshot, NativeSnapshot):
        raise RecoveryError('Invalid snapshot for managed projection')
    active_interfaces = {value['interface'] for identity, value in snapshot.objects.items()
                         if identity.kind == 'isis-binding'}
    objects = {}
    for identity, original in snapshot.objects.items():
        if identity.kind == 'isis-circuit' and identity.key not in active_interfaces:
            continue
        value = copy.deepcopy(original)
        if identity.kind == 'isis-srv6':
            value.pop('node_msd', None)
        objects[identity] = value
    return replace(snapshot, objects=objects,
                   unconfirmed=frozenset(snapshot.unconfirmed & objects.keys()))


def _merge_default_processes(text):
    """Merge only repeated canonical `router isis default` contexts."""
    output, bodies, active = [], [], None
    insertion = None
    for line in text.splitlines():
        if line == 'router isis default':
            if insertion is None:
                insertion = len(output)
                output.append(None)
            active = []
            bodies.append(active)
        elif active is not None and line and not line.startswith(' '):
            active = None
            if line != 'exit':
                output.append(line)
        elif active is not None:
            active.append(line)
        else:
            output.append(line)
    if insertion is None:
        return text

    # Reopening a process must not conceal conflicting scalar settings.
    seen = {}
    for body in bodies:
        indents = [len(line) - len(line.lstrip()) for line in body if line.strip()]
        shallow = min(indents) if indents else None
        for line in body:
            if len(line) - len(line.lstrip()) != shallow:
                continue
            command = line.strip()
            field = None
            for candidate, pattern in (
                    ('level', r'is-type \S+'),
                    ('topology', r'(?:no )?topology ipv6-unicast'),
                    ('hostname', r'(?:no )?hostname dynamic'),
                    ('mtu', r'lsp-mtu \S+'),
                    ('lifetime', r'max-lsp-lifetime \S+'),
                    ('refresh', r'lsp-refresh-interval \S+')):
                if re.fullmatch(pattern, command):
                    field = candidate
                    break
            if field is not None:
                if field in seen and seen[field] != command:
                    raise RecoveryError('Conflicting canonical IS-IS process setting')
                seen[field] = command
    output[insertion] = '\n'.join(['router isis default'] +
                                  [line for body in bodies for line in body])
    return '\n'.join(output)


def parse_canonical_config(text, defaults, frr_version):
    """Parse a trusted cfggen fragment with explicit release-selected defaults.

    The caller must validate the same ConfigDB snapshot used for this render.
    Unlike the native parser, this adapter supplies the cfggen-omitted framing
    and combines the default process contexts emitted by separate templates.
    """
    if not isinstance(text, str) or not text.strip():
        raise RecoveryError('Empty canonical configuration')
    frr_version = '.'.join(str(part) for part in normalize_version(frr_version))
    if any(line.startswith(('Building configuration', 'Current configuration:'))
           for line in text.splitlines()):
        raise RecoveryError('Native writer output is not a canonical render')
    headers = [line for line in text.splitlines() if line.startswith('frr version ')]
    if headers and headers != ['frr version ' + frr_version]:
        raise RecoveryError('Canonical render has a different FRR version')
    text = _merge_default_processes(text)
    if not headers:
        text = 'frr version ' + frr_version + '\n' + text
    text = text.rstrip() + '\nend\n'
    parsed = parse_running_config(text, defaults)
    if parsed.unsupported:
        raise RecoveryError('Unsupported canonical managed configuration')
    if any(identity.kind == 'isis-process' and identity.key != 'default'
           for identity in parsed.objects):
        raise RecoveryError('Canonical render contains a foreign IS-IS process')
    return project_managed_snapshot(parsed)
