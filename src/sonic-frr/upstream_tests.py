"""Run the original normal pytest, CUnit and Valgrind tests on native outputs."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET

from python.runfiles import runfiles


def merge_standalone_report(report, results):
    if not results:
        return
    tree = ET.parse(report) if report.exists() else ET.ElementTree(ET.Element('testsuites'))
    root = tree.getroot()
    if root.tag == 'testsuite':
        outer = ET.Element('testsuites')
        outer.append(root)
        root = outer
        tree = ET.ElementTree(root)
    suite = ET.SubElement(root, 'testsuite', name='pceplib', tests=str(len(results)), failures=str(sum(item['exit_code'] != 0 for item in results)), errors='0')
    for result in results:
        case = ET.SubElement(suite, 'testcase', name=result['name'], classname='pceplib', time=str(result['seconds']))
        if result['exit_code']:
            ET.SubElement(case, 'failure', message='Original check exited ' + str(result['exit_code'])).text = result['output']
    tree.write(report, encoding='utf-8', xml_declaration=True)


def retain_valgrind_logs(root, outputs, results):
    failed_logs = {
        result['name'].removesuffix('_valgrind.sh') + '.val.log'
        for result in results
        if result['exit_code'] and result['name'].endswith('_valgrind.sh')
    }
    for path in root.glob('pceplib/test/*.val.log'):
        if not path.is_file():
            continue
        destination = outputs / path.name
        if path.resolve() != destination.resolve():
            shutil.copyfile(path, destination)
        if path.relative_to(root).as_posix() in failed_logs:
            print('Valgrind diagnostics:', path.name, flush=True)
            print(path.read_text(errors='replace'), flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--python', required=True)
    parser.add_argument('--cli-reference', required=True)
    parser.add_argument('--config-status', required=True)
    parser.add_argument('--source', action='append', default=[])
    parser.add_argument('--binary', action='append', default=[])
    parser.add_argument('--standalone', action='append', default=[])
    options = parser.parse_args()
    declared = runfiles.Create()
    binaries = dict(value.split('=', 1) for value in options.binary)
    environment = dict(os.environ)
    for name in ['PYTHONHOME', 'PYTHONPATH', 'PYTHONSAFEPATH']:
        environment.pop(name, None)
    results = []
    with tempfile.TemporaryDirectory(prefix='frr-upstream-check-', dir=os.environ.get('TEST_TMPDIR')) as temporary:
        root = Path(temporary)
        tests = root / 'tests'
        tests.mkdir()
        for path in options.source:
            logical = path.split('/tests/', 1)[1]
            target = tests / logical
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(Path(declared.Rlocation(path)))
        # These two generated files preserve the upstream reference and feature
        # checks without coupling the test to the build's full configuration.
        reference = tests / 'lib/cli/test_cli.refout'
        reference.unlink(missing_ok=True)
        reference.symlink_to(Path(declared.Rlocation(options.cli_reference)))
        (root / 'config.status').symlink_to(Path(declared.Rlocation(options.config_status)))
        for logical, path in binaries.items():
            target = root / logical
            target.parent.mkdir(parents=True, exist_ok=True)
            # Preserve the helper layout while consuming the declared executable.
            target.symlink_to(Path(declared.Rlocation(path)))
        outputs = Path(os.environ.get('TEST_UNDECLARED_OUTPUTS_DIR', str(root / 'outputs')))
        outputs.mkdir(parents=True, exist_ok=True)
        for logical in options.standalone:
            if logical not in binaries:
                raise ValueError('Original standalone check has no declared native executable: ' + logical)
            if logical.endswith('_valgrind.sh'):
                # Write outside the temporary source layout from the start so
                # Bazel can retain diagnostics even if this test times out.
                log = root / (logical.removesuffix('_valgrind.sh') + '.val.log')
                log.symlink_to((outputs / log.name).resolve())
            started = time.monotonic()
            completed = subprocess.run([str(root / logical)], cwd=root, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            result = {'name': logical, 'exit_code': completed.returncode, 'seconds': time.monotonic() - started, 'output': completed.stdout}
            results.append(result)
            log = outputs / (logical + '.log')
            log.parent.mkdir(parents=True, exist_ok=True)
            log.write_text(completed.stdout)
            print('Original check:', logical, 'exit:', completed.returncode, flush=True)
            if completed.returncode:
                print(completed.stdout, flush=True)
        retain_valgrind_logs(root, outputs, results)
        report = Path(os.environ.get('XML_OUTPUT_FILE', str(outputs / 'tests.xml')))
        command = [declared.Rlocation(options.python), str(tests / 'runtests.py'), '--junitxml=' + str(report), '-v', str(tests)]
        pytest = subprocess.run(command, cwd=tests, env=environment)
        merge_standalone_report(report, results)
        (outputs / 'standalone-results.json').write_text(json.dumps(results, indent=2) + '\n')
        return int(pytest.returncode != 0 or any(result['exit_code'] for result in results))


if __name__ == '__main__':
    raise SystemExit(main())
