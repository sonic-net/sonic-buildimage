"""Helpers for unit-testing docker-fpm-frr host scripts (TSA/TSB/TSC/idf_isolation)."""
import json
import os
import subprocess
from pathlib import Path


BASE_IMAGE_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '../../../dockers/docker-fpm-frr/base_image_files')
)

SCRIPTS = ('TSA', 'TSB', 'TSC', 'idf_isolation', 'platform_utils')


def _write_executable(path, content):
    path.write_text(content)
    path.chmod(0o755)


def _rewrite_script(text):
    text = text.replace('[ "$EUID" -ne 0 ]', '[ "${TEST_EUID:-$EUID}" -ne 0 ]')
    text = text.replace('/usr/share/sonic/device/', '$TEST_ROOT/usr/share/sonic/device/')
    text = text.replace('/usr/bin/TS', '$TEST_ROOT/usr/bin/TS')
    text = text.replace('/bin/systemctl', 'systemctl')
    text = text.replace('/etc/sonic/', '$TEST_ROOT/etc/sonic/')
    return text


def build_script_env(tmp_path, extra_state=None):
    """Create rewritten scripts plus stub binaries for a self-contained run."""
    bin_dir = tmp_path / 'bin'
    bin_dir.mkdir()
    scripts_dir = tmp_path / 'scripts'
    scripts_dir.mkdir()
    test_root = tmp_path / 'root'
    platform_dir = test_root / 'usr/share/sonic/device/x86_64-test-r0'
    platform_dir.mkdir(parents=True)
    (test_root / 'etc/sonic').mkdir(parents=True)
    (test_root / 'usr/bin').mkdir(parents=True)

    state = {
        'default': {
            'platform': 'x86_64-test-r0',
            'type': 'SpineRouter',
            'subtype': '',
            'sub_role': 'FrontEnd',
            'tsa_enabled': 'false',
            'chassis_tsa_supported': 'false',
            'idf_isolation_state': 'unisolated',
        },
        'asic0': {
            'sub_role': 'FrontEnd',
            'idf_isolation_state': 'unisolated',
            'tsa_enabled': 'false',
        },
        'asic1': {
            'sub_role': 'BackEnd',
            'idf_isolation_state': 'unisolated',
            'tsa_enabled': 'false',
        },
        'asic2': {
            'sub_role': 'FrontEnd',
            'idf_isolation_state': 'isolated_no_export',
            'tsa_enabled': 'true',
        },
    }
    if extra_state:
        for key, value in extra_state.items():
            state.setdefault(key, {}).update(value)

    state_path = tmp_path / 'cfggen_state.json'
    writes_path = tmp_path / 'cfggen_writes.log'
    logger_path = tmp_path / 'logger.log'
    dbcli_path = tmp_path / 'dbcli.log'
    rexec_path = tmp_path / 'rexec.log'
    ts_path = tmp_path / 'ts.log'
    sudo_path = tmp_path / 'sudo.log'
    systemctl_path = tmp_path / 'systemctl.log'
    portstat_path = tmp_path / 'portstat.log'
    show_path = tmp_path / 'show.log'
    state_path.write_text(json.dumps(state))

    _write_executable(bin_dir / 'sonic-cfggen', CFGGEN_STUB)
    _write_executable(bin_dir / 'logger', '#!/bin/bash\nprintf "%s\\n" "$*" >> "$LOGGER_LOG"\n')
    _write_executable(bin_dir / 'sonic-db-cli', DBCLI_STUB)
    _write_executable(bin_dir / 'rexec', REXEC_STUB)
    _write_executable(bin_dir / 'sudo', '#!/bin/bash\nprintf "%s\\n" "$*" >> "$SUDO_LOG"\n')
    _write_executable(bin_dir / 'show', SHOW_STUB)
    _write_executable(bin_dir / 'portstat', '#!/bin/bash\nprintf "%s\\n" "$*" >> "$PORTSTAT_LOG"\n')
    _write_executable(bin_dir / 'systemctl', SYSTEMCTL_STUB)
    _write_executable(bin_dir / 'jq', '#!/bin/bash\necho "${JQ_DPU_COUNT:-0}"\n')
    _write_executable(test_root / 'usr/bin/TS', '#!/bin/bash\nprintf "%s\\n" "$*" >> "$TS_LOG"\necho "TS $*"\n')

    for name in SCRIPTS:
        source = Path(BASE_IMAGE_DIR) / name
        dest = scripts_dir / name
        dest.write_text(_rewrite_script(source.read_text()))
        dest.chmod(0o755)

    env = os.environ.copy()
    env['PATH'] = '{}:{}'.format(bin_dir, env.get('PATH', ''))
    env['TEST_EUID'] = '0'
    env['TEST_ROOT'] = str(test_root)
    env['CFGGEN_STATE'] = str(state_path)
    env['CFGGEN_WRITES'] = str(writes_path)
    env['LOGGER_LOG'] = str(logger_path)
    env['DBCLI_LOG'] = str(dbcli_path)
    env['REXEC_LOG'] = str(rexec_path)
    env['TS_LOG'] = str(ts_path)
    env['SUDO_LOG'] = str(sudo_path)
    env['SYSTEMCTL_LOG'] = str(systemctl_path)
    env['PORTSTAT_LOG'] = str(portstat_path)
    env['SHOW_LOG'] = str(show_path)
    env['JQ_DPU_COUNT'] = '0'
    env['SYSTEMCTL_ACTIVE'] = 'inactive'
    env['SYSTEMCTL_SUB'] = 'dead'
    env['REXEC_EXIT'] = '0'
    env['DBCLI_HDEL_EXIT'] = '0'
    env['MUX_STATUS'] = 'standby'
    env['SHOW_SUBTYPE'] = ''
    return {
        'env': env,
        'scripts': scripts_dir,
        'root': test_root,
        'platform_dir': platform_dir,
        'state_path': state_path,
        'writes_path': writes_path,
        'logger_path': logger_path,
        'dbcli_path': dbcli_path,
        'rexec_path': rexec_path,
        'ts_path': ts_path,
        'sudo_path': sudo_path,
        'systemctl_path': systemctl_path,
        'portstat_path': portstat_path,
        'show_path': show_path,
    }


def run_script(script_env, name, *args):
    return subprocess.run(
        [str(script_env['scripts'] / name)] + list(args),
        env=script_env['env'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def log_lines(path):
    if not Path(path).exists():
        return []
    return Path(path).read_text().splitlines()


def json_lines(path):
    return [json.loads(line) for line in log_lines(path) if line]


def load_state(script_env):
    with open(script_env['state_path']) as handle:
        return json.load(handle)


def save_state(script_env, state):
    script_env['state_path'].write_text(json.dumps(state))


def update_state(script_env, slot, **fields):
    state = load_state(script_env)
    state.setdefault(slot, {}).update(fields)
    save_state(script_env, state)


def write_platform_env(script_env, content):
    (script_env['platform_dir'] / 'platform_env.conf').write_text(content)


CFGGEN_STUB = r'''#!/usr/bin/env python3
import argparse
import json
import os
import sys

state_path = os.environ["CFGGEN_STATE"]
with open(state_path) as handle:
    state = json.load(handle)

parser = argparse.ArgumentParser()
parser.add_argument("-d", action="store_true")
parser.add_argument("-H", action="store_true")
parser.add_argument("-v", dest="var")
parser.add_argument("-n", dest="ns")
parser.add_argument("-a", dest="apply")
parser.add_argument("-w", action="store_true")
args = parser.parse_args()

def slot(ns):
    return state.get(ns or "default", state.get("default", {}))

if args.apply:
    payload = json.loads(args.apply)["BGP_DEVICE_GLOBAL"]["STATE"]
    key = args.ns or "default"
    state.setdefault(key, {}).update(payload)
    with open(state_path, "w") as handle:
        json.dump(state, handle)
    writes_path = os.environ.get("CFGGEN_WRITES")
    if writes_path:
        with open(writes_path, "a") as handle:
            handle.write(json.dumps({"ns": args.ns, "state": payload}) + "\n")
    sys.exit(0)

var = args.var or ""
values = slot(args.ns)
if var == "DEVICE_METADATA.localhost.type":
    print(values.get("type", "SpineRouter"))
elif var == "DEVICE_METADATA.localhost.platform":
    print(values.get("platform", "x86_64-test-r0"))
elif var == "DEVICE_METADATA.localhost.subtype":
    print(values.get("subtype", ""))
elif "sub_role" in var:
    print(values.get("sub_role", ""))
elif "idf_isolation_state" in var:
    print(values.get("idf_isolation_state", "unisolated"))
elif "chassis_tsa_supported" in var:
    print(values.get("chassis_tsa_supported", "false"))
elif "tsa_enabled" in var:
    print(values.get("tsa_enabled", "false"))
'''

DBCLI_STUB = r'''#!/bin/bash
printf "%s\n" "$*" >> "$DBCLI_LOG"
if [[ "$1" == "CONFIG_DB" && "$2" == "hget" ]]; then
    python3 - <<'PY'
import json, os
state = json.load(open(os.environ["CFGGEN_STATE"]))
print(state.get("default", {}).get("subtype", os.environ.get("SHOW_SUBTYPE", "")))
PY
    exit 0
fi
if [[ "$1" == "CHASSIS_APP_DB" && "$2" == "HGET" ]]; then
    python3 - <<'PY'
import json, os
state = json.load(open(os.environ["CFGGEN_STATE"]))
print(state.get("default", {}).get("tsa_enabled", "false"))
PY
    exit 0
fi
if [[ "$1" == "STATE_DB" && "$2" == "HDEL" ]]; then
    exit "${DBCLI_HDEL_EXIT:-0}"
fi
exit 0
'''

REXEC_STUB = r'''#!/bin/bash
printf "%s\n" "$*" >> "$REXEC_LOG"
exit "${REXEC_EXIT:-0}"
'''

SHOW_STUB = r'''#!/bin/bash
printf "%s\n" "$*" >> "$SHOW_LOG"
if [[ "$1" == "mux" ]]; then
    echo "${MUX_STATUS}"
fi
'''

SYSTEMCTL_STUB = r'''#!/bin/bash
printf "%s\n" "$*" >> "$SYSTEMCTL_LOG"
if [[ "$*" == *"ActiveState"* ]]; then
    echo "${SYSTEMCTL_ACTIVE:-inactive}"
elif [[ "$*" == *"SubState"* ]]; then
    echo "${SYSTEMCTL_SUB:-dead}"
elif [[ "$*" == *"ActiveEnterTimestamp"* ]]; then
    echo "Thu 2026-01-01 00:00:00 UTC"
fi
exit 0
'''
