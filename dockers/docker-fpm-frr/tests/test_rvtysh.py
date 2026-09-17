import os
import subprocess
from pathlib import Path

import pytest


RVTYSH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', 'base_image_files', 'rvtysh')
)


@pytest.fixture
def vtysh_stub(tmp_path):
    bin_dir = tmp_path / 'bin'
    bin_dir.mkdir()
    capture_path = tmp_path / 'argv'
    stub_path = bin_dir / 'vtysh'
    stub_path.write_text(
        '#!/bin/bash\n'
        'printf "%s\\0" "$@" > "$VTYSH_CAPTURE"\n'
        'printf "%s" "${VTYSH_PAGER-unset}" > "$VTYSH_CAPTURE.pager"\n'
        'printf "%s" "${PAGER-unset}" > "$VTYSH_CAPTURE.pager_fallback"\n'
    )
    stub_path.chmod(0o755)

    wrapper_path = tmp_path / 'rvtysh'
    wrapper_path.write_text(
        Path(RVTYSH).read_text().replace(
            'readonly VTYSH=/usr/bin/vtysh',
            'readonly VTYSH={}'.format(stub_path),
        )
    )
    wrapper_path.chmod(0o755)

    env = os.environ.copy()
    env['VTYSH_CAPTURE'] = str(capture_path)
    env['VTYSH_PAGER'] = 'malicious pager'
    env['PAGER'] = 'malicious fallback pager'
    return env, capture_path, wrapper_path


def run_rvtysh(arguments, vtysh_stub, cwd=None):
    env, capture_path, wrapper_path = vtysh_stub
    result = subprocess.run(
        [str(wrapper_path)] + arguments,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    captured_arguments = []
    if capture_path.exists():
        captured_arguments = [
            value.decode()
            for value in capture_path.read_bytes().split(b'\0')
            if value
        ]
    return result, captured_arguments


@pytest.mark.parametrize(
    'arguments, expected',
    [
        (['-c', 'show version'], ['-c', 'show version']),
        (
            ['-n', '0', '-c', 'show ip bgp summary'],
            ['-n', '0', '-c', 'show ip bgp summary'],
        ),
        (
            ['-c', 'show version', '-c', 'show ip route'],
            ['-c', 'show version', '-c', 'show ip route'],
        ),
        (
            ['-c', 'show ip route ?'],
            ['-c', 'show ip route ?'],
        ),
        (
            ['-c', 'show ip route summ?'],
            ['-c', 'show ip route summ?'],
        ),
        (
            ['-n', '10', '-c', 'show version'],
            ['-n', '10', '-c', 'show version'],
        ),
    ],
)
def test_allows_only_valid_show_commands(vtysh_stub, arguments, expected):
    result, captured_arguments = run_rvtysh(arguments, vtysh_stub)

    assert result.returncode == 0
    assert result.stdout == ''
    assert result.stderr == ''
    assert captured_arguments == expected


@pytest.mark.parametrize(
    'arguments',
    [
        [],
        ['-f', '/tmp/config'],
        ['-b'],
        ['-d', 'bgpd', '-c', 'show version'],
        ['-c'],
        ['-c', 'configure terminal'],
        ['-c', 'showfoo'],
        ['-c', 'show version; configure terminal'],
        ['-c', 'show run | include password'],
        ['-c', 'show version\nconfigure terminal'],
        ['-c', 'show version', '-c', 'configure terminal'],
        ['-n'],
        ['-n', 'x', '-c', 'show version'],
        ['-n', '0', '-n', '1', '-c', 'show version'],
        ['-c', 'show version', 'unexpected'],
    ],
)
def test_rejects_invalid_or_ambiguous_arguments(vtysh_stub, arguments):
    result, captured_arguments = run_rvtysh(arguments, vtysh_stub)

    assert result.returncode == 1
    assert result.stdout == ''
    assert result.stderr == (
        'Not allowed to run command. Please run sudo vtysh instead.\n'
    )
    assert captured_arguments == []


def test_preserves_whitespace_and_glob_characters(vtysh_stub, tmp_path):
    (tmp_path / 'ip').touch()
    command = 'show  *'

    result, captured_arguments = run_rvtysh(
        ['-c', command],
        vtysh_stub,
        cwd=tmp_path,
    )

    assert result.returncode == 0
    assert captured_arguments == ['-c', command]


def test_removes_pager_environment(vtysh_stub):
    result, captured_arguments = run_rvtysh(
        ['-c', 'show version'],
        vtysh_stub,
    )
    _, capture_path, _ = vtysh_stub

    assert result.returncode == 0
    assert captured_arguments == ['-c', 'show version']
    assert capture_path.with_suffix('.pager').read_text() == 'unset'
    assert capture_path.with_suffix('.pager_fallback').read_text() == 'unset'
