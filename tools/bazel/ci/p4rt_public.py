#!/usr/bin/env python3
"""Select public P4RT outputs without publishing private build or workspace paths."""

import argparse
import json
from pathlib import Path
import shutil
import sys
import xml.etree.ElementTree as ElementTree

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci.public_artifacts import (
    bep_summary, dependency_json, label, load_json, safe_json,
)
from tools.bazel.ci.python_packages import test_result


def owner_test_summary(directory):
    """Summarize retained unified @sonic_pins XML and BEP statuses without copying logs."""
    receipt = json.loads((directory / 'owner-test-results.json').read_text())
    check.require(receipt.get('status') == 'passed', 'Owner test receipt did not pass')
    targets = receipt.get('targets', [])
    check.require(isinstance(targets, list) and targets and len(set(targets)) == len(targets),
                  'Expected a unique nonempty owner test selection')
    statuses = receipt.get('results', {})
    check.require(isinstance(statuses, dict) and statuses and
                  all(value == 'passed' for value in statuses.values()) and
                  len(statuses) == len(targets),
                  'Every selected owner test needs a passing status')
    tests_root = (directory / 'tests').resolve()
    results, total_bytes = {}, 0
    for target in sorted(targets):
        label(target)
        check.require('p4rt_app' in target and target.count(':') == 1,
                      'Unexpected owner test target: ' + target)
        name = target.rsplit(':', 1)[1]
        xml_path = directory / 'tests' / name / 'test.xml'
        check.require(xml_path.is_file() and not xml_path.is_symlink() and
                      xml_path.resolve().is_relative_to(tests_root),
                      'Owner test XML escaped its retained directory: ' + name)
        size = xml_path.stat().st_size
        total_bytes += size
        check.require(0 < size <= 2 * 1024**2 and total_bytes <= 16 * 1024**2,
                      'Owner test XML exceeds the summary size limit')
        counts = test_result(xml_path)
        check.require(counts['skipped'] == 0 and not ElementTree.parse(xml_path).getroot().findall('.//skipped'),
                      'Owner tests must execute without skipped cases')
        results[target] = {'status': 'PASSED', 'xml_sha256': check.sha(xml_path), **counts}
    return {'status': 'passed', 'owner_revision': receipt['owner_revision'],
            'target_count': len(results), 'testcase_count': sum(x['tests'] for x in results.values()),
            'tests': results}


def prepare(directory):
    directory.mkdir(parents=True, exist_ok=True)
    public = directory / 'public'
    # Never upload leftovers from an earlier attempt as current evidence.
    public.mkdir(exist_ok=False)

    def write(name, value):
        safe_json(value)
        (public / name).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')

    receipt_path = directory / 'build-receipt.json'
    if receipt_path.exists():
        receipt = json.loads(receipt_path.read_text())
        write('public-receipt.json', {key: receipt[key] for key in
              ('status', 'architecture', 'revision', 'owner_revision', 'artifacts') if key in receipt})
        for name, record in receipt.get('artifacts', {}).items():
            source = directory / name
            check.require(Path(name).name == name and not source.is_symlink(), 'Unexpected public artifact path')
            check.require(check.sha(source) == record['sha256'], 'Public artifact changed: ' + name)
            if source.suffix == '.json':
                safe_json(json.loads(source.read_text()))
            shutil.copyfile(source, public / name)
    for name in ('validation.json', 'action-audit.json', 'incremental.json', 'source-pins.json', 'resolved-modules.json'):
        source = directory / name
        if source.is_file():
            write(name, json.loads(source.read_text()))
    for source in directory.glob('*.validation.json'):
        write(source.name, json.loads(source.read_text()))
    for name, kind in (('MODULE.bazel.lock', 'lock'), ('module-graph.json', 'graph')):
        source = directory / name
        if source.is_file():
            dependency_json(json.loads(source.read_text()), kind)
            shutil.copyfile(source, public / name)
    tests_path = directory / 'owner-test-results.json'
    if tests_path.exists():
        source = json.loads(tests_path.read_text())
        write('owner-tests.json', {key: source[key] for key in ('status', 'owner_revision', 'targets', 'results') if key in source})
        if source.get('status') == 'passed':
            write('owner-test-summary.json', owner_test_summary(directory))
    summary = {str(path.relative_to(directory)): bep_summary(path.read_text().splitlines(), partial=True)
               for path in directory.rglob('*events.json*') if not path.is_relative_to(public)}
    write('build-summary.json', summary)
    return public


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    print(prepare(args.directory.resolve()))
