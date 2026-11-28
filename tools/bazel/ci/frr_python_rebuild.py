#!/usr/bin/env python3
"""Prove FRR Python wheel/TAR no-op stability and source/template invalidation."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


COMPONENTS = ('sonic-bgpcfgd', 'sonic-frr-mgmt-framework')
TARGETS = {
    component: ['//src/' + component + ':' + component.replace('-', '_') + '_wheel',
                '//src/' + component + ':runtime']
    for component in COMPONENTS
}
INPUTS = {
    'sonic-bgpcfgd': ('src/sonic-bgpcfgd/bgpcfgd/directory.py', b'\n# Bazel declared-source rebuild probe.\n'),
    'sonic-frr-mgmt-framework': ('src/sonic-frr-mgmt-framework/templates/bfdd/bfdd.conf.j2',
                                 b'\n! Bazel declared-template rebuild probe\n'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bazel', default='bazel')
    parser.add_argument('--bazel-arg', action='append', default=[])
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    receipt = {'status': 'running', 'scope': 'same output base; not a cold-build or remote-cache-hit proof',
               'phases': {}}
    originals = {component: (root / item[0]).read_bytes() for component, item in INPUTS.items()}

    def snapshot(phase):
        start = time.monotonic()
        subprocess.run([args.bazel, 'build', *args.bazel_arg,
                        *[target for targets in TARGETS.values() for target in targets]],
                       cwd=root, check=True)
        hashes = {}
        for targets in TARGETS.values():
            for target in targets:
                paths = subprocess.check_output(
                    [args.bazel, 'cquery', *args.bazel_arg, '--output=files', target],
                    cwd=root, text=True).splitlines()
                if len(paths) != 1:
                    raise ValueError('Expected one output: ' + target)
                hashes[target] = hashlib.sha256((root / paths[0]).read_bytes()).hexdigest()
        receipt['phases'][phase] = {'sha256': hashes, 'seconds': round(time.monotonic() - start, 3)}
        return hashes

    try:
        baseline = snapshot('baseline')
        if snapshot('noop') != baseline:
            raise ValueError('No-op build changed a wheel or TAR')
        for component, (name, addition) in INPUTS.items():
            path = root / name
            try:
                path.write_bytes(originals[component] + addition)
                changed = snapshot(component + '-edit')
                expected = set(TARGETS[component])
                actual = {target for target in baseline if changed[target] != baseline[target]}
                if actual != expected:
                    raise ValueError('Edit did not change precisely its wheel and TAR: ' + component)
            finally:
                path.write_bytes(originals[component])
            if snapshot(component + '-restore') != baseline:
                raise ValueError('Restoring source did not restore package bytes: ' + component)
        receipt['status'] = 'passed'
    except Exception:
        receipt['status'] = 'failed'
        raise
    finally:
        for component, (name, _) in INPUTS.items():
            (root / name).write_bytes(originals[component])
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
