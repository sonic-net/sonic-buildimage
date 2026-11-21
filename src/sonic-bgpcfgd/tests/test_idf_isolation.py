import pytest

from .base_image_script_util import (
    json_lines,
    load_state,
    log_lines,
    run_script,
    update_state,
    write_platform_env,
)


VALID_STATES = ('isolated_no_export', 'isolated_withdraw_all', 'unisolated')
CONFIG_SAVE_HINT = (
    "Please execute 'sudo config save' to preserve IDF isolation state "
    "after reboot or config reload\n"
)


def run_idf(script_env, *args):
    return run_script(script_env, 'idf_isolation', *args)


def test_requires_root(script_env):
    script_env['env']['TEST_EUID'] = '1000'

    result = run_idf(script_env, 'status')

    assert result.returncode == 1
    assert result.stdout == 'Root priveleges are needed for this operation\n'
    assert json_lines(script_env['writes_path']) == []


def test_rejects_non_spine_router(script_env):
    update_state(script_env, 'default', type='ToRRouter')

    result = run_idf(script_env, 'status')

    assert result.returncode == 1
    assert result.stdout == 'Operation is not supported on this platform\n'
    assert json_lines(script_env['writes_path']) == []


@pytest.mark.parametrize(
    'platform_env',
    [
        'supervisor=1\ndisaggregated_chassis=1\n',
        'supervisor=0\ndisaggregated_chassis=0\n',
        '',
    ],
)
def test_skips_chassis_supervisor(script_env, platform_env):
    (script_env['root'] / 'etc/sonic/chassisdb.conf').write_text('enabled=1\n')
    write_platform_env(script_env, platform_env)

    result = run_idf(script_env, 'isolated_no_export')

    assert result.returncode == 0
    assert result.stdout == 'Skipping Operation on chassis supervisor\n'
    assert json_lines(script_env['writes_path']) == []


def test_allows_disaggregated_linecard(script_env):
    (script_env['root'] / 'etc/sonic/chassisdb.conf').write_text('enabled=1\n')
    write_platform_env(script_env, 'supervisor=0\ndisaggregated_chassis=1\n')

    result = run_idf(script_env, 'status')

    assert result.returncode == 0
    assert 'Skipping Operation on chassis supervisor' not in result.stdout
    assert result.stdout == 'IDF isolation state: unisolated\n'


@pytest.mark.parametrize('argument', ['', 'isolate', 'ISOLATED_NO_EXPORT', 'statuss'])
def test_rejects_invalid_argument(script_env, argument):
    args = [argument] if argument else []

    result = run_idf(script_env, *args)

    assert result.returncode == 0
    assert 'Invalid parameter {}, Operation not supported'.format(argument) in result.stdout
    assert (
        'Usage: sudo idf_isolation '
        '<isolated_no_export|isolated_withdraw_all|unisolated|status>'
    ) in result.stdout
    assert json_lines(script_env['writes_path']) == []


def test_status_single_asic(script_env):
    result = run_idf(script_env, 'status')

    assert result.returncode == 0
    assert result.stdout == 'IDF isolation state: unisolated\n'
    assert CONFIG_SAVE_HINT not in result.stdout
    assert json_lines(script_env['writes_path']) == []
    assert log_lines(script_env['logger_path']) == []


@pytest.mark.parametrize('state', VALID_STATES)
def test_single_asic_already_in_requested_state(script_env, state):
    update_state(script_env, 'default', idf_isolation_state=state)

    result = run_idf(script_env, state)

    assert result.returncode == 0
    assert result.stdout == (
        'IDF is already in {} state\n{}'.format(state, CONFIG_SAVE_HINT)
    )
    assert json_lines(script_env['writes_path']) == []


@pytest.mark.parametrize('state', VALID_STATES)
def test_single_asic_applies_new_state(script_env, state):
    update_state(
        script_env,
        'default',
        idf_isolation_state='unisolated' if state != 'unisolated' else 'isolated_no_export',
    )

    result = run_idf(script_env, state)

    assert result.returncode == 0
    assert result.stdout == (
        'IDF isolation state: {}\n{}'.format(state, CONFIG_SAVE_HINT)
    )
    assert json_lines(script_env['writes_path']) == [{'ns': None, 'state': {'idf_isolation_state': state}}]
    assert log_lines(script_env['logger_path']) == [
        '-t {} -p user.info IDF isolation state: {}'.format(state, state)
    ]
    assert load_state(script_env)['default']['idf_isolation_state'] == state


def test_multi_asic_status_and_update_frontend_only(script_env):
    (script_env['platform_dir'] / 'asic.conf').write_text('NUM_ASIC=3\n')

    status = run_idf(script_env, 'status')

    assert status.returncode == 0
    assert status.stdout == (
        'BGP0: IDF isolation state: unisolated\n'
        'BGP2: IDF isolation state: isolated_no_export\n'
    )

    result = run_idf(script_env, 'isolated_no_export')

    assert result.returncode == 0
    assert result.stdout == (
        'BGP0: IDF isolation state: isolated_no_export\n'
        'BGP2: IDF is already in isolated_no_export state\n'
        '{}'.format(CONFIG_SAVE_HINT)
    )
    assert json_lines(script_env['writes_path']) == [
        {'ns': 'asic0', 'state': {'idf_isolation_state': 'isolated_no_export'}}
    ]
    state = load_state(script_env)
    assert state['asic0']['idf_isolation_state'] == 'isolated_no_export'
    assert state['asic1']['idf_isolation_state'] == 'unisolated'
    assert state['asic2']['idf_isolation_state'] == 'isolated_no_export'


def test_multi_asic_unisolate_frontend_asics(script_env):
    (script_env['platform_dir'] / 'asic.conf').write_text('NUM_ASIC=3\n')
    update_state(script_env, 'asic0', idf_isolation_state='isolated_withdraw_all')
    update_state(script_env, 'asic2', idf_isolation_state='isolated_withdraw_all')

    result = run_idf(script_env, 'unisolated')

    assert result.returncode == 0
    assert result.stdout == (
        'BGP0: IDF isolation state: unisolated\n'
        'BGP2: IDF isolation state: unisolated\n'
        '{}'.format(CONFIG_SAVE_HINT)
    )
    assert json_lines(script_env['writes_path']) == [
        {'ns': 'asic0', 'state': {'idf_isolation_state': 'unisolated'}},
        {'ns': 'asic2', 'state': {'idf_isolation_state': 'unisolated'}},
    ]
