"""Check Redis alias metadata and full Bash startup without a switch."""

import json
import os
from pathlib import Path
import runpy
import shlex
import subprocess
import sys
import types
from unittest.mock import MagicMock

import pytest

ROOT = Path(__file__).resolve().parents[3]
GENERATOR = ROOT / 'files/scripts/sonic-db-aliases.py'
BASHRC = ROOT / 'files/image_config/bash/bash.bashrc'


@pytest.mark.parametrize('port, socket', [(6379, None), (6400, None), (0, '/run/redis/redis.sock'),
                                        (0, "/run/redis/a b:'quoted'.sock")])
def test_generator_reports_both_available_endpoints(monkeypatch, capsys, port, socket):
    config = types.SimpleNamespace(getDbId=lambda db: 4, getDbPort=lambda db: port,
                                   getDbSock=MagicMock(return_value=socket))
    module = types.ModuleType('swsscommon.swsscommon')
    module.SonicDBConfig = config
    package = types.ModuleType('swsscommon')
    package.swsscommon = module
    monkeypatch.setitem(sys.modules, 'swsscommon', package)
    monkeypatch.setitem(sys.modules, 'swsscommon.swsscommon', module)
    monkeypatch.setattr(sys, 'argv', [str(GENERATOR), 'CONFIG_DB'])
    runpy.run_path(str(GENERATOR), run_name='__main__')
    expected = f'CONFIG_DB:4:{port}:{socket or ""}'
    assert capsys.readouterr().out.strip() == expected
    config.getDbSock.assert_called_once_with("CONFIG_DB")


def test_missing_socket_leaves_socket_field_empty(monkeypatch, capsys):
    config = types.SimpleNamespace(getDbId=lambda db: 4, getDbPort=lambda db: 0,
                                   getDbSock=lambda db: '')
    package = types.ModuleType('swsscommon')
    module = types.ModuleType('swsscommon.swsscommon')
    module.SonicDBConfig = config
    package.swsscommon = module
    monkeypatch.setitem(sys.modules, 'swsscommon', package)
    monkeypatch.setitem(sys.modules, 'swsscommon.swsscommon', module)
    monkeypatch.setattr(sys, 'argv', [str(GENERATOR), 'CONFIG_DB'])
    runpy.run_path(str(GENERATOR), run_name='__main__')
    captured = capsys.readouterr()
    assert captured.out.strip() == 'CONFIG_DB:4:0:'
    assert captured.err == ''


@pytest.mark.parametrize('interactive', [False, True])
@pytest.mark.parametrize('endpoint', ['6379', '0:/run/redis/a b.sock',
                                    "0:/run/redis/a:'quoted'.sock", '0:/run/redis/[literal]*.sock'])
def test_actual_bash_startup_preserves_native_arguments(tmp_path, interactive, endpoint):
    generator = tmp_path / 'generator'
    generator.write_text("#!/bin/sh\nprintf '%s\\n' " + shlex.quote('CONFIG_DB:4:' + endpoint) + '\n')
    generator.chmod(0o755)
    capture = tmp_path / 'args.json'
    native = tmp_path / 'redis-cli'
    native.write_text('#!/usr/bin/python3\nimport json,os,sys\n'
                      'open(os.environ["ALIAS_CAPTURE"],"w").write(json.dumps(sys.argv[1:]))\n')
    native.chmod(0o755)
    bashrc = tmp_path / 'bashrc'
    bashrc.write_text(BASHRC.read_text().replace('/usr/local/bin/sonic-db-aliases.py', shlex.quote(str(generator))))
    environment = dict(os.environ, PATH=str(tmp_path) + ':' + os.environ['PATH'], ALIAS_CAPTURE=str(capture))
    environment.pop('SSH_TARGET_CONSOLE_LINE', None)
    query = "redis-configdb{} --scan --pattern 'x y*'".format('' if endpoint == '6379' else '-unix')
    if interactive:
        command = ['/bin/bash', '--noprofile', '--rcfile', str(bashrc), '-i', '-c', query]
    else:
        command = ['/bin/bash', '-c', 'PS1=test\nshopt -s expand_aliases\nsource ' + shlex.quote(str(bashrc)) + '\n' + query]
    result = subprocess.run(command, env=environment, cwd=tmp_path, capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr
    expected = ['-n', '4', '-p', endpoint] if endpoint == '6379' else ['-n', '4', '-s', endpoint[2:]]
    assert json.loads(capture.read_text()) == expected + ['--scan', '--pattern', 'x y*']


def test_noninteractive_startup_without_prompt_keeps_existing_guard(tmp_path):
    result = subprocess.run(['/bin/bash', '--noprofile', '-c',
                             'unset PS1\nsource ' + shlex.quote(str(BASHRC)) + '\ntype redis-configdb'],
                            capture_output=True, text=True, timeout=10)
    assert result.returncode != 0
    assert 'not found' in result.stderr
