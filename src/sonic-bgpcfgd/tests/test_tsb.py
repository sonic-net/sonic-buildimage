from .base_image_script_util import (
    json_lines,
    load_state,
    log_lines,
    run_script,
    update_state,
    write_platform_env,
)


def run_tsb(script_env, *args):
    return run_script(script_env, 'TSB', *args)


def test_requires_root(script_env):
    script_env['env']['TEST_EUID'] = '1000'

    result = run_tsb(script_env)

    assert result.returncode == 1
    assert result.stdout == 'Root priveleges are needed for this operation\n'
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_chassis_tsa_already_normal(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='true', tsa_enabled='false')

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert 'Chassis is already in Normal mode' in result.stdout
    assert 'sudo config save' in result.stdout
    assert json_lines(script_env['writes_path']) == []
    assert log_lines(script_env['logger_path']) == [
        '-t TSB -p user.info Chassis is already in Normal mode'
    ]
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_chassis_tsa_maintenance_to_normal(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='true', tsa_enabled='true')

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert 'Chassis Mode: Maintenance -> Normal' in result.stdout
    assert json_lines(script_env['writes_path']) == [
        {'ns': None, 'state': {'tsa_enabled': 'false'}}
    ]
    assert load_state(script_env)['default']['tsa_enabled'] == 'false'
    assert log_lines(script_env['ts_path']) == []


def test_supervisor_rexec_to_linecards(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    update_state(script_env, 'default', chassis_tsa_supported='false', tsa_enabled='true')

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert 'Chassis Mode: Maintenance -> Normal' in result.stdout
    assert log_lines(script_env['rexec_path']) == ['all -c sudo TSB chassis']
    assert json_lines(script_env['writes_path']) == [
        {'ns': None, 'state': {'tsa_enabled': 'false'}}
    ]


def test_supervisor_rexec_failure_is_logged(script_env):
    write_platform_env(script_env, 'supervisor=1\n')
    script_env['env']['REXEC_EXIT'] = '1'

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert any('rexec TSB to linecards failed' in line
               for line in log_lines(script_env['logger_path']))


def test_linecard_invokes_ts_and_prints_save_hint(script_env):
    result = run_tsb(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSB']
    assert 'Please execute \'sudo config save\' to preserve System mode in Normal state' in result.stdout


def test_linecard_chassis_argument_skips_save_hint(script_env):
    result = run_tsb(script_env, 'chassis')

    assert result.returncode == 0
    assert log_lines(script_env['ts_path']) == ['TSB']
    assert 'sudo config save' not in result.stdout


def test_dualtor_toggles_mux_to_auto(script_env):
    update_state(script_env, 'default', subtype='DualToR')

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert log_lines(script_env['sudo_path']) == ['config mux mode auto all']
    assert any('Toggle all mux mode to auto' in line
               for line in log_lines(script_env['logger_path']))


def test_stops_startup_tsa_tsb_service(script_env):
    script_env['env']['SYSTEMCTL_ACTIVE'] = 'active'
    script_env['env']['SYSTEMCTL_SUB'] = 'running'

    result = run_tsb(script_env)

    assert result.returncode == 0
    assert 'Stopping startup_tsa_tsb.service before configuring TSB' in result.stdout
    assert 'Successfully removed TSA-TSB service flag.' in result.stdout
    assert any(line.startswith('stop startup_tsa_tsb.service')
               for line in log_lines(script_env['systemctl_path']))
