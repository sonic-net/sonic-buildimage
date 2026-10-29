# cli-plugin-tests/test_idf_isolation.py
#
# Unit tests for dockers/docker-fpm-frr/base_image_files/idf_isolation.
# slave.mk runs pytest in this directory while building docker-fpm-frr.

import os
import subprocess

import pytest

IDF_ISOLATION_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', 'base_image_files', 'idf_isolation')
)

_CFGGEN_STUB = r'''#!/bin/bash
echo "$@" >> "$CFGGEN_LOG"
write=false
ns=""
var=""
payload=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -w) write=true; shift ;;
        -n) ns="$2"; shift 2 ;;
        -v) var="$2"; shift 2 ;;
        -a) payload="$2"; shift 2 ;;
        *) shift ;;
    esac
done
if $write; then
    echo "${ns}|${payload}" >> "$CFGGEN_WRITE_LOG"
    exit 0
fi
if [[ "$var" == *localhost.type* ]]; then
    echo "${MOCK_DEVICE_TYPE:-SpineRouter}"
elif [[ "$var" == *localhost.platform* ]]; then
    echo "${MOCK_PLATFORM:-x86_64-test}"
elif [[ "$var" == *sub_role* ]]; then
    key="MOCK_SUB_ROLE_${ns}"
    echo "${!key:-${MOCK_SUB_ROLE:-FrontEnd}}"
elif [[ "$var" == *idf_isolation_state* ]]; then
    if [[ -n "$ns" ]]; then
        key="MOCK_IDF_STATE_${ns}"
        echo "${!key:-${MOCK_IDF_STATE:-unisolated}}"
    else
        echo "${MOCK_IDF_STATE:-unisolated}"
    fi
fi
'''

_LOGGER_STUB = r'''#!/bin/bash
echo "$@" >> "$LOGGER_LOG"
'''


def _make_harness(tmp_path):
    """Runnable copy of idf_isolation with mocked helpers and overridable paths."""
    script = tmp_path / 'idf_isolation'
    text = open(IDF_ISOLATION_PATH).read()
    text = text.replace('$EUID', '${MOCK_EUID:-$EUID}')
    text = text.replace(
        '/etc/sonic/chassisdb.conf',
        '${CHASSISDB_CONF:-/nonexistent/chassisdb.conf}',
    )
    text = text.replace(
        'PLATFORM_ENV_CONF_FILE=/usr/share/sonic/device/$PLATFORM/platform_env.conf',
        'PLATFORM_ENV_CONF_FILE=${PLATFORM_ENV_CONF_FILE:-/usr/share/sonic/device/$PLATFORM/platform_env.conf}',
    )
    text = text.replace(
        'ASIC_CONF=/usr/share/sonic/device/$PLATFORM/asic.conf',
        'ASIC_CONF=${ASIC_CONF:-/usr/share/sonic/device/$PLATFORM/asic.conf}',
    )
    text = text.replace(
        '/etc/sonic/sonic-environment',
        '${SONIC_ENVIRONMENT:-/nonexistent/sonic-environment}',
    )
    script.write_text(text)
    script.chmod(0o755)

    bindir = tmp_path / 'bin'
    bindir.mkdir()
    (bindir / 'sonic-cfggen').write_text(_CFGGEN_STUB)
    (bindir / 'sonic-cfggen').chmod(0o755)
    (bindir / 'logger').write_text(_LOGGER_STUB)
    (bindir / 'logger').chmod(0o755)

    write_log = tmp_path / 'cfggen_write.log'
    logger_log = tmp_path / 'logger.log'
    env = os.environ.copy()
    env['PATH'] = str(bindir) + os.pathsep + env.get('PATH', '')
    env['MOCK_EUID'] = '0'
    env['MOCK_DEVICE_TYPE'] = 'SpineRouter'
    env['MOCK_IDF_STATE'] = 'unisolated'
    env['PLATFORM'] = 'x86_64-test'
    env['CHASSISDB_CONF'] = str(tmp_path / 'missing-chassisdb.conf')
    env['PLATFORM_ENV_CONF_FILE'] = str(tmp_path / 'missing-platform_env.conf')
    env['ASIC_CONF'] = str(tmp_path / 'missing-asic.conf')
    env['SONIC_ENVIRONMENT'] = str(tmp_path / 'missing-sonic-environment')
    env['CFGGEN_LOG'] = str(tmp_path / 'cfggen.log')
    env['CFGGEN_WRITE_LOG'] = str(write_log)
    env['LOGGER_LOG'] = str(logger_log)
    return {
        'script': script,
        'env': env,
        'write_log': write_log,
        'logger_log': logger_log,
    }


def _run(tmp_path, args, extra_env=None):
    harness = _make_harness(tmp_path)
    env = harness['env']
    if extra_env:
        env.update(extra_env)
    result = subprocess.run(
        [str(harness['script'])] + list(args),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    writes = harness['write_log'].read_text() if harness['write_log'].exists() else ''
    logs = harness['logger_log'].read_text() if harness['logger_log'].exists() else ''
    return result, writes, logs


def _state_json(state):
    return '{"BGP_DEVICE_GLOBAL":{"STATE":{"idf_isolation_state": "%s"}}}' % state


def test_requires_root(tmp_path):
    result, writes, _ = _run(tmp_path, ['isolated_no_export'], extra_env={'MOCK_EUID': '1000'})
    assert result.returncode == 1
    assert 'Root priveleges are needed for this operation' in result.stdout
    assert writes == ''


def test_rejects_non_spine(tmp_path):
    result, writes, _ = _run(
        tmp_path, ['isolated_no_export'], extra_env={'MOCK_DEVICE_TYPE': 'LeafRouter'}
    )
    assert result.returncode == 1
    assert 'Operation is not supported on this platform' in result.stdout
    assert writes == ''


@pytest.mark.parametrize('role', ['SpineRouter', 'LowerSpineRouter', 'UpperSpineRouter'])
def test_allows_spine_roles(tmp_path, role):
    result, writes, logs = _run(
        tmp_path, ['isolated_no_export'], extra_env={'MOCK_DEVICE_TYPE': role}
    )
    assert result.returncode == 0
    assert 'IDF isolation state: isolated_no_export' in result.stdout
    assert _state_json('isolated_no_export') in writes
    assert 'IDF isolation state: isolated_no_export' in logs


def test_rejects_invalid_parameter(tmp_path):
    result, writes, _ = _run(tmp_path, ['not_a_state'])
    assert result.returncode == 0
    assert 'Invalid parameter not_a_state, Operation not supported' in result.stdout
    assert 'Usage: sudo idf_isolation' in result.stdout
    assert writes == ''


def test_status_single_asic(tmp_path):
    result, writes, logs = _run(
        tmp_path, ['status'], extra_env={'MOCK_IDF_STATE': 'isolated_no_export'}
    )
    assert result.returncode == 0
    assert 'IDF isolation state: isolated_no_export' in result.stdout
    assert "Please execute 'sudo config save'" not in result.stdout
    assert writes == ''
    assert logs == ''


@pytest.mark.parametrize('state', ['isolated_no_export', 'isolated_withdraw_all', 'unisolated'])
def test_apply_state_single_asic(tmp_path, state):
    current = 'isolated_no_export' if state == 'unisolated' else 'unisolated'
    result, writes, logs = _run(tmp_path, [state], extra_env={'MOCK_IDF_STATE': current})
    assert result.returncode == 0
    assert 'IDF isolation state: %s' % state in result.stdout
    assert "Please execute 'sudo config save'" in result.stdout
    assert writes.strip() == '|%s' % _state_json(state)
    assert 'IDF isolation state: %s' % state in logs


def test_already_in_state(tmp_path):
    result, writes, logs = _run(
        tmp_path,
        ['isolated_no_export'],
        extra_env={'MOCK_IDF_STATE': 'isolated_no_export'},
    )
    assert result.returncode == 0
    assert 'IDF is already in isolated_no_export state' in result.stdout
    assert "Please execute 'sudo config save'" in result.stdout
    assert writes == ''
    assert logs == ''


def test_skips_chassis_supervisor(tmp_path):
    chassisdb = tmp_path / 'chassisdb.conf'
    chassisdb.write_text('')
    result, writes, _ = _run(
        tmp_path,
        ['isolated_no_export'],
        extra_env={'CHASSISDB_CONF': str(chassisdb)},
    )
    assert result.returncode == 0
    assert 'Skipping Operation on chassis supervisor' in result.stdout
    assert writes == ''


def test_skips_disaggregated_supervisor(tmp_path):
    chassisdb = tmp_path / 'chassisdb.conf'
    chassisdb.write_text('')
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('disaggregated_chassis=1\nsupervisor=1\n')
    result, writes, _ = _run(
        tmp_path,
        ['isolated_no_export'],
        extra_env={
            'CHASSISDB_CONF': str(chassisdb),
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
        },
    )
    assert result.returncode == 0
    assert 'Skipping Operation on chassis supervisor' in result.stdout
    assert writes == ''


def test_applies_on_disaggregated_linecard(tmp_path):
    chassisdb = tmp_path / 'chassisdb.conf'
    chassisdb.write_text('')
    platform_env = tmp_path / 'platform_env.conf'
    platform_env.write_text('disaggregated_chassis=1\n')
    result, writes, logs = _run(
        tmp_path,
        ['isolated_withdraw_all'],
        extra_env={
            'CHASSISDB_CONF': str(chassisdb),
            'PLATFORM_ENV_CONF_FILE': str(platform_env),
        },
    )
    assert result.returncode == 0
    assert 'Skipping Operation on chassis supervisor' not in result.stdout
    assert 'IDF isolation state: isolated_withdraw_all' in result.stdout
    assert _state_json('isolated_withdraw_all') in writes
    assert 'IDF isolation state: isolated_withdraw_all' in logs


def test_multi_asic_frontend_only(tmp_path):
    asic_conf = tmp_path / 'asic.conf'
    asic_conf.write_text('NUM_ASIC=2\n')
    result, writes, logs = _run(
        tmp_path,
        ['isolated_no_export'],
        extra_env={
            'ASIC_CONF': str(asic_conf),
            'MOCK_SUB_ROLE_asic0': 'FrontEnd',
            'MOCK_SUB_ROLE_asic1': 'BackEnd',
            'MOCK_IDF_STATE_asic0': 'unisolated',
            'MOCK_IDF_STATE_asic1': 'unisolated',
        },
    )
    assert result.returncode == 0
    assert 'BGP0: IDF isolation state: isolated_no_export' in result.stdout
    assert 'BGP1:' not in result.stdout
    assert writes.strip() == 'asic0|%s' % _state_json('isolated_no_export')
    assert 'IDF isolation state: isolated_no_export' in logs


def test_multi_asic_status(tmp_path):
    asic_conf = tmp_path / 'asic.conf'
    asic_conf.write_text('NUM_ASIC=2\n')
    result, writes, logs = _run(
        tmp_path,
        ['status'],
        extra_env={
            'ASIC_CONF': str(asic_conf),
            'MOCK_SUB_ROLE_asic0': 'FrontEnd',
            'MOCK_SUB_ROLE_asic1': 'FrontEnd',
            'MOCK_IDF_STATE_asic0': 'isolated_no_export',
            'MOCK_IDF_STATE_asic1': 'unisolated',
        },
    )
    assert result.returncode == 0
    assert 'BGP0: IDF isolation state: isolated_no_export' in result.stdout
    assert 'BGP1: IDF isolation state: unisolated' in result.stdout
    assert "Please execute 'sudo config save'" not in result.stdout
    assert writes == ''
    assert logs == ''
