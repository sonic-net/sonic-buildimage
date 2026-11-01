# cli-plugin-tests/test_tsb.py
#
# Unit tests for dockers/docker-fpm-frr/base_image_files/TSB.
# slave.mk runs pytest in this directory while building docker-fpm-frr.

import os
import subprocess

import pytest

BASE_IMAGE_FILES = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', 'base_image_files')
)
TSB_PATH = os.path.join(BASE_IMAGE_FILES, 'TSB')
PLATFORM_UTILS_PATH = os.path.join(BASE_IMAGE_FILES, 'platform_utils')

TSB_STATE_JSON = '{"BGP_DEVICE_GLOBAL":{"STATE":{"tsa_enabled": "false"}}}'

_CFGGEN_STUB = r'''#!/bin/bash
echo "$@" >> "$CFGGEN_LOG"
write=false
var=""
payload=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -w) write=true; shift ;;
        -v) var="$2"; shift 2 ;;
        -a) payload="$2"; shift 2 ;;
        *) shift ;;
    esac
done
if $write; then
    echo "$payload" >> "$CFGGEN_WRITE_LOG"
    exit 0
fi
if [[ "$var" == *localhost.platform* ]]; then
    echo "${MOCK_PLATFORM:-x86_64-test}"
elif [[ "$var" == *localhost.subtype* ]]; then
    echo "${MOCK_SUBTYPE:-}"
elif [[ "$var" == *chassis_tsa_supported* ]]; then
    echo "${MOCK_CHASSIS_TSA_SUPPORTED:-false}"
elif [[ "$var" == *tsa_enabled* ]]; then
    echo "${MOCK_TSA_ENABLED:-false}"
fi
'''

_LOGGER_STUB = r'''#!/bin/bash
echo "$@" >> "$LOGGER_LOG"
'''

_TS_STUB = r'''#!/bin/bash
echo "$@" >> "$TS_LOG"
echo "${MOCK_TS_OUTPUT:-System Mode: Maintenance -> Normal}"
'''

_SYSTEMCTL_STUB = r'''#!/bin/bash
echo "$@" >> "$SYSTEMCTL_LOG"
if [[ "$*" == *ActiveState* ]]; then
    echo "${MOCK_SERVICE_ACTIVE:-inactive}"
elif [[ "$*" == *SubState* ]]; then
    echo "${MOCK_SERVICE_SUB:-dead}"
fi
'''

_DBCLI_STUB = r'''#!/bin/bash
echo "$@" >> "$DBCLI_LOG"
if [[ "$*" == *hget* && "$*" == *subtype* ]]; then
    echo "${MOCK_SUBTYPE:-}"
elif [[ "$*" == *HDEL* && "$*" == *tsa_tsb_service* ]]; then
    exit ${MOCK_HDEL_RC:-0}
fi
exit 0
'''

_REXEC_STUB = r'''#!/bin/bash
echo "$@" >> "$REXEC_LOG"
exit ${MOCK_REXEC_RC:-0}
'''

_SUDO_STUB = r'''#!/bin/bash
echo "$@" >> "$SUDO_LOG"
'''

_JQ_STUB = r'''#!/bin/bash
echo "${MOCK_NUM_DPU:-0}"
'''


def _write_stub(path, contents):
    path.write_text(contents)
    path.chmod(0o755)


def _make_harness(tmp_path):
    """Runnable TSB + platform_utils with mocked helpers and overridable paths."""
    platform_utils = tmp_path / 'platform_utils'
    platform_utils_text = open(PLATFORM_UTILS_PATH).read()
    platform_utils_text = platform_utils_text.replace(
        'PLATFORM_JSON=/usr/share/sonic/device/$PLATFORM/platform.json',
        'PLATFORM_JSON=${PLATFORM_JSON:-/usr/share/sonic/device/$PLATFORM/platform.json}',
    )
    platform_utils.write_text(platform_utils_text)
    platform_utils.chmod(0o755)

    script = tmp_path / 'TSB'
    text = open(TSB_PATH).read()
    text = text.replace('$EUID', '${MOCK_EUID:-$EUID}')
    text = text.replace(
        'PLATFORM_ENV_CONF_FILE=/usr/share/sonic/device/$PLATFORM/platform_env.conf',
        'PLATFORM_ENV_CONF_FILE=${PLATFORM_ENV_CONF_FILE:-/usr/share/sonic/device/$PLATFORM/platform_env.conf}',
    )
    text = text.replace('/usr/bin/TS', '${TS_BIN:-/usr/bin/TS}')
    text = text.replace('/bin/systemctl', '${SYSTEMCTL_BIN:-/bin/systemctl}')
    script.write_text(text)
    script.chmod(0o755)

    bindir = tmp_path / 'bin'
    bindir.mkdir()
    _write_stub(bindir / 'sonic-cfggen', _CFGGEN_STUB)
    _write_stub(bindir / 'logger', _LOGGER_STUB)
    _write_stub(bindir / 'TS', _TS_STUB)
    _write_stub(bindir / 'systemctl', _SYSTEMCTL_STUB)
    _write_stub(bindir / 'sonic-db-cli', _DBCLI_STUB)
    _write_stub(bindir / 'rexec', _REXEC_STUB)
    _write_stub(bindir / 'sudo', _SUDO_STUB)
    _write_stub(bindir / 'jq', _JQ_STUB)

    env = os.environ.copy()
    env['PATH'] = str(bindir) + os.pathsep + env.get('PATH', '')
    env['MOCK_EUID'] = '0'
    env['MOCK_PLATFORM'] = 'x86_64-test'
    env['MOCK_SUBTYPE'] = ''
    env['MOCK_TSA_ENABLED'] = 'true'
    env['MOCK_CHASSIS_TSA_SUPPORTED'] = 'false'
    env['MOCK_SERVICE_ACTIVE'] = 'inactive'
    env['MOCK_SERVICE_SUB'] = 'dead'
    env['MOCK_NUM_DPU'] = '0'
    env['PLATFORM_ENV_CONF_FILE'] = str(tmp_path / 'missing-platform_env.conf')
    env['PLATFORM_JSON'] = str(tmp_path / 'missing-platform.json')
    env['TS_BIN'] = str(bindir / 'TS')
    env['SYSTEMCTL_BIN'] = str(bindir / 'systemctl')
    env['CFGGEN_LOG'] = str(tmp_path / 'cfggen.log')
    env['CFGGEN_WRITE_LOG'] = str(tmp_path / 'cfggen_write.log')
    env['LOGGER_LOG'] = str(tmp_path / 'logger.log')
    env['TS_LOG'] = str(tmp_path / 'ts.log')
    env['SYSTEMCTL_LOG'] = str(tmp_path / 'systemctl.log')
    env['DBCLI_LOG'] = str(tmp_path / 'dbcli.log')
    env['REXEC_LOG'] = str(tmp_path / 'rexec.log')
    env['SUDO_LOG'] = str(tmp_path / 'sudo.log')
    return {
        'script': script,
        'env': env,
        'tmp_path': tmp_path,
        'bindir': bindir,
    }


def _read(path):
    return path.read_text() if path.exists() else ''


def _run(tmp_path, args=None, extra_env=None):
    harness = _make_harness(tmp_path)
    env = harness['env']
    if extra_env:
        env.update(extra_env)
    result = subprocess.run(
        [str(harness['script'])] + list(args or []),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    logs = {
        'writes': _read(tmp_path / 'cfggen_write.log'),
        'logger': _read(tmp_path / 'logger.log'),
        'ts': _read(tmp_path / 'ts.log'),
        'systemctl': _read(tmp_path / 'systemctl.log'),
        'dbcli': _read(tmp_path / 'dbcli.log'),
        'rexec': _read(tmp_path / 'rexec.log'),
        'sudo': _read(tmp_path / 'sudo.log'),
    }
    return result, logs


def test_requires_root(tmp_path):
    result, logs = _run(tmp_path, extra_env={'MOCK_EUID': '1000'})
    assert result.returncode == 1
    assert 'Root priveleges are needed for this operation' in result.stdout
    assert logs['ts'] == ''
    assert logs['writes'] == ''


def test_pizza_box_invokes_ts_and_config_save(tmp_path):
    result, logs = _run(tmp_path)
    assert result.returncode == 0
    assert logs['ts'].strip() == 'TSB'
    assert "Please execute 'sudo config save' to preserve System mode in Normal" in result.stdout
    assert 'WARNING' not in result.stdout
    assert logs['rexec'] == ''
    assert logs['writes'] == ''


def test_chassis_arg_skips_config_save(tmp_path):
    result, logs = _run(tmp_path, args=['chassis'])
    assert result.returncode == 0
    assert logs['ts'].strip() == 'TSB'
    assert "Please execute 'sudo config save'" not in result.stdout
    assert 'WARNING' not in result.stdout


@pytest.mark.parametrize('subtype', ['UpstreamLC', 'DownstreamLC'])
def test_linecard_does_not_warn_to_run_tsb_on_supervisor(tmp_path, subtype):
    result, logs = _run(tmp_path, extra_env={'MOCK_SUBTYPE': subtype})
    assert result.returncode == 0
    assert logs['ts'].strip() == 'TSB'
    assert 'WARNING' not in result.stdout
    assert 'Supervisor' not in result.stdout


def test_supervisor_chassis_tsa_already_in_normal(tmp_path):
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('supervisor=1\n')
    result, logs = _run(
        tmp_path,
        extra_env={
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
            'MOCK_CHASSIS_TSA_SUPPORTED': 'true',
            'MOCK_TSA_ENABLED': 'false',
        },
    )
    assert result.returncode == 0
    assert 'Chassis is already in Normal mode' in result.stdout
    assert "sudo config save" in result.stdout
    assert logs['ts'] == ''
    assert logs['writes'] == ''
    assert 'Chassis is already in Normal mode' in logs['logger']


def test_supervisor_chassis_tsa_supported_apply(tmp_path):
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('supervisor=1\n')
    result, logs = _run(
        tmp_path,
        extra_env={
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
            'MOCK_CHASSIS_TSA_SUPPORTED': 'true',
            'MOCK_TSA_ENABLED': 'true',
        },
    )
    assert result.returncode == 0
    assert 'Chassis Mode: Maintenance -> Normal' in result.stdout
    assert TSB_STATE_JSON in logs['writes']
    assert 'CHASSIS_APP_DB' in logs['dbcli']
    assert 'tsa_enabled' in logs['dbcli']
    assert logs['ts'] == ''
    assert logs['rexec'] == ''
    assert 'Chassis Mode: Maintenance -> Normal' in logs['logger']


def test_supervisor_rexec_when_chassis_tsa_not_supported(tmp_path):
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('supervisor=1\n')
    result, logs = _run(
        tmp_path,
        extra_env={
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
            'MOCK_CHASSIS_TSA_SUPPORTED': 'false',
        },
    )
    assert result.returncode == 0
    assert 'Chassis Mode: Maintenance -> Normal' in result.stdout
    assert TSB_STATE_JSON in logs['writes']
    assert 'all -c sudo TSB chassis' in logs['rexec'].replace("'", '')
    assert 'rexec all' in result.stdout
    assert logs['ts'] == ''


def test_rexec_failure_is_logged(tmp_path):
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('supervisor=1\n')
    result, logs = _run(
        tmp_path,
        extra_env={
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
            'MOCK_CHASSIS_TSA_SUPPORTED': 'false',
            'MOCK_REXEC_RC': '1',
        },
    )
    assert result.returncode == 0
    assert 'rexec TSB to linecards failed' in logs['logger']


def test_dualtor_toggles_mux_auto(tmp_path):
    result, logs = _run(tmp_path, extra_env={'MOCK_SUBTYPE': 'DualToR'})
    assert result.returncode == 0
    assert 'config mux mode auto all' in logs['sudo']
    assert 'Toggle all mux mode to auto' in logs['logger']
    assert logs['ts'].strip() == 'TSB'


def test_non_dualtor_does_not_toggle_mux(tmp_path):
    result, logs = _run(tmp_path, extra_env={'MOCK_SUBTYPE': 'SpineRouter'})
    assert result.returncode == 0
    assert logs['sudo'] == ''
    assert logs['ts'].strip() == 'TSB'


def test_stops_startup_tsa_tsb_service(tmp_path):
    result, logs = _run(
        tmp_path,
        extra_env={
            'MOCK_SERVICE_ACTIVE': 'active',
            'MOCK_SERVICE_SUB': 'running',
            'MOCK_HDEL_RC': '0',
        },
    )
    assert result.returncode == 0
    assert 'Stopping startup_tsa_tsb.service before configuring TSB' in result.stdout
    assert 'Successfully removed TSA-TSB service flag.' in result.stdout
    assert 'stop startup_tsa_tsb.service' in logs['systemctl']
    assert 'HDEL' in logs['dbcli']
    assert logs['ts'].strip() == 'TSB'


def test_service_flag_removal_failure_is_reported(tmp_path):
    result, logs = _run(
        tmp_path,
        extra_env={
            'MOCK_SERVICE_ACTIVE': 'active',
            'MOCK_SERVICE_SUB': 'running',
            'MOCK_HDEL_RC': '1',
        },
    )
    assert result.returncode == 0
    assert 'Failed to remove TSA-TSB service flag!' in result.stderr
    assert logs['ts'].strip() == 'TSB'


def test_skips_stop_service_when_started_by_service(tmp_path):
    result, logs = _run(
        tmp_path,
        extra_env={
            'STARTED_BY_TSA_TSB_SERVICE': '1',
            'MOCK_SERVICE_ACTIVE': 'active',
            'MOCK_SERVICE_SUB': 'running',
        },
    )
    assert result.returncode == 0
    assert 'Stopping startup_tsa_tsb.service' not in result.stdout
    assert logs['systemctl'] == ''
    assert logs['ts'].strip() == 'TSB'


def test_smartswitch_supervisor_does_not_take_chassis_path(tmp_path):
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('supervisor=1\n')
    platform_json = tmp_path / 'platform.json'
    platform_json.write_text('{"DPUS": {"dpu0": {}}}')
    result, logs = _run(
        tmp_path,
        extra_env={
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
            'PLATFORM_JSON': str(platform_json),
            'MOCK_NUM_DPU': '1',
            'MOCK_CHASSIS_TSA_SUPPORTED': 'true',
        },
    )
    assert result.returncode == 0
    assert logs['ts'].strip() == 'TSB'
    assert 'Chassis Mode' not in result.stdout
    assert logs['rexec'] == ''
