"""Execute the production startup shell function with bounded source doubles.

The cfggen double uses the actual SRv6 Jinja template and a small BGP sentinel;
it is not native cfggen/FRR validation. Python startup validation is real. All
filesystem writes stay in pytest's temporary directory or the function's own
temporary directory. Run this harness on the selected Linux worker.
"""

import json
import os
from pathlib import Path
import subprocess
import sys

import pytest


FRAMEWORK = Path(__file__).resolve().parents[1]
DOCKER_INIT = FRAMEWORK.parents[1] / 'dockers/docker-fpm-frr/docker_init.sh'


CFGGEN_DOUBLE = r'''
import json
import os
from pathlib import Path
import stat
import sys
from jinja2 import Environment, FileSystemLoader, StrictUndefined

args = sys.argv[1:]
event = {'kind': 'cfggen', 'args': args}
if '-d' in args:
    # A capture must not execute templates before its snapshot is available.
    if '-t' in args or '--print-data' not in args:
        sys.exit(21)
    source = Path(os.environ['HARNESS_DATABASE'])
    config = json.loads(source.read_text())
    changed = json.loads(source.read_text())
    changed['BGP_GLOBALS']['default']['local_asn'] = '65099'
    source.write_text(json.dumps(changed))
    with open(os.environ['HARNESS_EVENTS'], 'a') as stream:
        stream.write(json.dumps(event) + '\n')
    print(json.dumps(config))
    sys.exit(0)
source = Path(args[args.index('-j') + 1])
config = json.loads(source.read_text())
event.update(source=str(source), mode=stat.S_IMODE(source.stat().st_mode),
             tables=sorted(config))
with open(os.environ['HARNESS_EVENTS'], 'a') as stream:
    stream.write(json.dumps(event) + '\n')
target = args[args.index('-t') + 1].split(',', 1)[1]
destination = Path(os.environ['HARNESS_DESTINATION'] if target == '/etc/frr/frr.conf' else target)
# Exercise cleanup of a partially written candidate on template failure.
destination.write_text('partial candidate\n')
environment = Environment(loader=FileSystemLoader(os.environ['HARNESS_TEMPLATES']),
                          trim_blocks=True, undefined=StrictUndefined)
output = []
for vrf, row in config.get('ISIS_GLOBALS', {}).items():
    if row.get('enabled') == 'true':
        output.append('router isis ' + vrf + ('' if vrf == 'default' else ' vrf ' + vrf))
output.append(environment.get_template('srv6.conf.j2').render(**config))
output.append('router bgp ' + config['BGP_GLOBALS']['default']['local_asn'])
output.append(' neighbor 192.0.2.1 password ' + config['BGP_NEIGHBOR']['192.0.2.1']['password'])
destination.write_text('\n'.join(output) + '\n')
'''


PYTHON_LAUNCHER = r'''
import json
import os
from pathlib import Path
import stat
import sys

args = sys.argv[1:]
if args[:2] == ['-m', 'frrcfgd.isis_startup']:
    snapshot = Path(args[args.index('--snapshot') + 1])
    candidate = Path(args[args.index('--rendered-config') + 1])
    event = {'kind': 'helper', 'candidate_exists': candidate.exists(),
             'source': str(snapshot), 'mode': stat.S_IMODE(snapshot.stat().st_mode)}
    with open(os.environ['HARNESS_EVENTS'], 'a') as stream:
        stream.write(json.dumps(event) + '\n')
    args += ['--journal', os.environ['HARNESS_JOURNAL']]
os.execv(sys.executable, [sys.executable] + args)
'''


def executable(path, program):
    path.write_text('#!' + sys.executable + '\n' + program)
    path.chmod(0o755)


@pytest.mark.parametrize('problem', ['missing-prefix', 'arg-len', 'legacy-binding', 'namespace'])
def test_shell_managed_failure_keeps_frozen_bgp_and_cleans_secrets(tmp_path, problem):
    binaries = tmp_path / 'bin'
    binaries.mkdir()
    executable(binaries / 'sonic-cfggen', CFGGEN_DOUBLE)
    executable(binaries / 'python3', PYTHON_LAUNCHER)
    executable(binaries / 'zebra', 'print("zebra version 10.5.4")\n')
    executable(binaries / 'jq',
               'import json,sys\nprint(json.load(open(sys.argv[-1])).get("profile", "canonical"))\n')
    config = {
        'PORT': {'Ethernet0': {}}, 'LOOPBACK_INTERFACE': {'Loopback0': {}},
        'ISIS_GLOBALS': {'default': {'enabled': 'true', 'srv6_locator': 'LOC1',
                                     'srv6_interface': 'Loopback0'}},
        'ISIS_INTERFACE': {'Ethernet0': {'enabled': 'true'}},
        'SRV6_MY_LOCATORS': {'LOC1': {}},
        'BGP_GLOBALS': {'default': {'local_asn': '65000'}},
        'BGP_NEIGHBOR': {'192.0.2.1': {'password': 'frozen-secret'}},
    }
    if problem == 'arg-len':
        config['SRV6_MY_LOCATORS']['LOC1'] = {'prefix': '2001:db8:100::', 'arg_len': '16'}
    elif problem == 'legacy-binding':
        config['ISIS_GLOBALS'] = {'Blue': {'enabled': 'true', 'srv6_locator': 'LOC1'}}
    database, destination, events, journal = [tmp_path / name for name in
                                              ('database.json', 'final.conf', 'events.jsonl', 'owned.json')]
    database.write_text(json.dumps(config))
    # Execute the actual production function, without the script's host setup.
    source = DOCKER_INIT.read_text()
    begin = source.index('render_integrated_frr_config()\n(\n')
    end = source.index('\n)\n', begin) + len('\n)\n')
    runner = tmp_path / 'run.sh'
    # Bind command names explicitly: the worker also has an installed cfggen.
    # PATH-only injection must never let this source harness call that binary.
    commands = ''.join(name + '() { "$HARNESS_BIN/' + name + '" "$@"; }\n'
                       for name in ('sonic-cfggen', 'python3', 'zebra', 'jq'))
    runner.write_text(source[begin:end] + '\n' + commands +
                      'CFGGEN_PARAMS="-d -y /etc/sonic/constants.yml '
                      '-t /usr/share/sonic/templates/gen_frr.conf.j2,/etc/frr/frr.conf"\n'
                      'render_integrated_frr_config\n')
    environment = dict(os.environ, PATH=str(binaries) + ':' + os.environ['PATH'],
                       HARNESS_BIN=str(binaries),
                       HARNESS_DATABASE=str(database), HARNESS_DESTINATION=str(destination),
                       HARNESS_EVENTS=str(events), HARNESS_JOURNAL=str(journal),
                       HARNESS_TEMPLATES=str(FRAMEWORK / 'templates/srv6'),
                       NAMESPACE_ID='0' if problem == 'namespace' else '')
    result = subprocess.run(['bash', str(runner)], cwd=FRAMEWORK, env=environment,
                            capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, result.stderr
    output = destination.read_text()
    assert 'router bgp 65000' in output and '65099' not in output
    assert 'password frozen-secret' in output
    assert 'segment-routing' not in output and 'partial candidate' not in output
    assert ('router isis' in output) == (problem in ('legacy-binding', 'namespace'))
    assert 'frozen-secret' not in result.stdout + result.stderr
    trace = [json.loads(line) for line in events.read_text().splitlines()]
    captures = [event for event in trace if event['kind'] == 'cfggen' and '-d' in event['args']]
    renders = [event for event in trace if event['kind'] == 'cfggen' and '-j' in event['args']]
    helpers = [event for event in trace if event['kind'] == 'helper']
    assert len(captures) == 1 and '-t' not in captures[0]['args']
    assert len(renders) == 2 and len(helpers) == 1
    assert helpers[0]['candidate_exists'] == (problem in ('arg-len', 'legacy-binding'))
    assert helpers[0]['source'] == renders[0]['source']
    assert all(event['mode'] == 0o600 for event in renders + helpers)
    assert not Path(helpers[0]['source']).parent.exists()
    assert not journal.exists()
