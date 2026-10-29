from .base_image_script_util import (
    log_lines,
    run_script,
    update_state,
    write_platform_env,
)


def run_tsc(script_env, *args):
    return run_script(script_env, 'TSC', *args)


def test_requires_root(script_env):
    script_env['env']['TEST_EUID'] = '1000'

    result = run_tsc(script_env)

    assert result.returncode == 1
    assert result.stdout == 'Root priveleges are needed for this operation\n'
    assert log_lines(script_env['ts_path']) == []
    assert log_lines(script_env['rexec_path']) == []


def test_supervisor_rexec_tsc(script_env):
    write_platform_env(script_env, 'supervisor=1\n')

    result = run_tsc(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['rexec_path']) == ['all -c sudo TSC']
    assert log_lines(script_env['ts_path']) == []
    assert log_lines(script_env['portstat_path']) == []


def test_supervisor_rexec_tsc_no_stats(script_env):
    write_platform_env(script_env, 'supervisor=1\n')

    result = run_tsc(script_env, 'no-stats')

    assert result.returncode == 0
    assert log_lines(script_env['rexec_path']) == ['all -c sudo TSC no-stats']
    assert log_lines(script_env['ts_path']) == []


def test_linecard_invokes_ts_and_portstat(script_env):
    result = run_tsc(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSC']
    assert log_lines(script_env['portstat_path']) == ['-p 5']


def test_linecard_no_stats_skips_portstat(script_env):
    result = run_tsc(script_env, 'no-stats')

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSC']
    assert log_lines(script_env['portstat_path']) == []


def test_dualtor_prints_mux_status(script_env):
    update_state(script_env, 'default', subtype='DualToR')
    script_env['env']['MUX_STATUS'] = 'Ethernet0 standby'

    result = run_tsc(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['show_path']) == ['mux status']
    assert 'Ethernet0 standby' in result.stdout
