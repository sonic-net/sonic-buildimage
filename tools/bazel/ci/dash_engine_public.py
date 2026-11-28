#!/usr/bin/env python3
"""Review the explicit public DASH engine evidence without private command logs."""

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.dash_engine_image import TARGETS, IMAGES
from tools.bazel.ci.public_artifacts import safe_json, dependency_json


def review(directory):
    names = {name for name in TARGETS if name.endswith('.json')}
    names.update(('validation.json', 'action-audit.json', 'owner-action-audit.json', 'incremental.json'))
    names.update(name + '.validation.json' for name in IMAGES)
    for name in sorted(names):
        path = directory / name
        if path.exists():
            safe_json(json.loads(path.read_text()))
    for name, kind in (('MODULE.bazel.lock', 'lock'), ('module-graph.json', 'graph')):
        path = directory / name
        if path.exists():
            dependency_json(json.loads(path.read_text()), kind)
    receipt = directory / 'build-receipt.json'
    if receipt.exists():
        data = json.loads(receipt.read_text())
        public = {key: data[key] for key in ('status', 'revision', 'architecture', 'image_version', 'artifacts', 'tested_revision_parents') if key in data}
        safe_json(public)
        (directory / 'public-receipt.json').write_text(json.dumps(public, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifacts', type=Path, required=True)
    review(parser.parse_args().artifacts)
