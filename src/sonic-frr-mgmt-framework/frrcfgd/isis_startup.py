"""Register startup ownership before exposing a rendered IS-IS fragment.

Adapted from gs1571's trusted-preapply snapshot/ownership protocol. Full and
fallback renders consume the same frozen ConfigDB input. BGP is not coupled to
successful IS-IS preparation.
"""
import argparse
import json
import os
from pathlib import Path
import time
from .isis_config import desired, render
from .isis_recovery import AtomicJSONStore, prepare_startup_record, read_boot_id


def prepare(snapshot, store, clock=time.time, boot_id_supplier=read_boot_id):
    state = desired(snapshot)
    lines = render(snapshot)
    with store.guard():
        record = prepare_startup_record(store.load(), state, 'isis-northbound-v1', clock(), 30,
                                        trusted_preapply=True, boot_id=boot_id_supplier())
        store.save(record)
        if record.status not in ('prepared', 'pending'):
            raise ValueError('Startup ownership not authorized: ' + record.reason)
    return dict(snapshot, ISIS_CONFIG_LINES=lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--snapshot', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--journal', default='/etc/frr/isis-owned.json')
    args = parser.parse_args()
    snapshot = json.loads(Path(args.snapshot).read_text())
    status = 0
    try:
        enriched = prepare(snapshot, AtomicJSONStore(args.journal))
    except Exception as error:
        import sys
        print('IS-IS startup unavailable: {}'.format(error), file=sys.stderr)
        enriched, status = dict(snapshot, ISIS_CONFIG_LINES=[]), 1
    descriptor = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(descriptor, 'w') as output:
        json.dump(enriched, output)
    return status


if __name__ == '__main__':
    raise SystemExit(main())
