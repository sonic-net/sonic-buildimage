"""Check the image build permission independently of topology selection."""

import json
import os
from pathlib import Path
import subprocess
import sys
from unittest import mock

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_startup as startup


@pytest.fixture
def boot(root_directory, monkeypatch):
    marker = root_directory / 'build-enabled'
    marker.write_text('n\n')
    marker.chmod(0o644)
    intent = root_directory / 'redis-auth.json'
    intent.write_text(json.dumps({'mode': 'enforce'}))
    intent.chmod(0o600)
    monkeypatch.setattr(startup, 'BUILD_SETTING', marker)
    return marker, intent, root_directory / 'runtime'


@pytest.mark.parametrize('content', ['{"mode":"enforce"}', '{'])
def test_disabled_build_ignores_restored_activation_and_creates_no_runtime(boot, content):
    marker, intent, runtime = boot
    intent.write_text(content)
    with mock.patch.object(startup, 'selection', side_effect=AssertionError('must not read activation')):
        assert startup.prepare_selected(intent_path=intent, runtime=runtime) == []
        assert not startup.runtime_required(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_missing_build_setting_keeps_legacy(boot):
    marker, intent, runtime = boot
    marker.unlink()
    assert not startup.build_enabled()
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == []
    assert not startup.runtime_required(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_enabled_build_allows_explicit_selection(boot):
    marker, intent, runtime = boot
    marker.write_text('y\n')
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['redis', 'redis_bmp']
    assert startup.runtime_required(intent_path=intent, runtime=runtime)
    inode = (runtime / 'local-system/writer.secret').stat().st_ino
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['redis', 'redis_bmp']
    assert (runtime / 'local-system/writer.secret').stat().st_ino == inode


@pytest.mark.parametrize('value', ['', 'invalid', 'Y'])
def test_invalid_build_setting_blocks_startup(boot, value):
    marker, intent, runtime = boot
    marker.write_text(value)
    with pytest.raises(startup.CredentialStateError, match='build setting'):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


@pytest.mark.parametrize('mode', [0o664, 0o666])
def test_writable_build_setting_is_rejected(boot, mode):
    marker, _, _ = boot
    marker.chmod(mode)
    with pytest.raises(startup.CredentialStateError, match='unsafe'):
        startup.build_enabled()


def test_build_setting_symlink_is_rejected(boot):
    marker, _, _ = boot
    target = marker.with_name('target')
    marker.rename(target)
    marker.symlink_to(target)
    with pytest.raises(OSError):
        startup.build_enabled()


def test_untrusted_build_setting_owner_is_rejected(boot):
    marker, _, _ = boot
    os.chown(marker, 65534, 65534)
    with pytest.raises(startup.CredentialStateError, match='unsafe'):
        startup.build_enabled()


@pytest.mark.parametrize('remove', [False, True])
def test_selected_boot_cannot_downgrade_when_build_setting_changes(boot, remove):
    marker, intent, runtime = boot
    marker.write_text('y\n')
    startup.prepare_selected(intent_path=intent, runtime=runtime)
    if remove:
        marker.unlink()
    else:
        marker.write_text('n\n')
    for operation in (startup.prepare_selected, startup.runtime_required):
        with pytest.raises(startup.CredentialStateError, match='activation changed'):
            operation(intent_path=intent, runtime=runtime)


def test_image_build_option_default_and_installation(tmp_path):
    root = Path(__file__).resolve().parents[4]
    assert 'ENABLE_REDIS_WRITE_PROTECTION ?= y' in (root / 'rules/config').read_text()
    assert 'ENABLE_REDIS_WRITE_PROTECTION=$(ENABLE_REDIS_WRITE_PROTECTION)' in (root / 'Makefile.work').read_text()
    assert 'export ENABLE_REDIS_WRITE_PROTECTION' in (root / 'slave.mk').read_text()
    template = (root / 'files/build_templates/sonic_debian_extension.j2').read_text()
    start = template.index('# Install Redis password preparation')
    end = template.index('{% if include_system_eventd', start)
    script = template[start:end]
    image = tmp_path / 'image'
    (image / 'usr/bin').mkdir(parents=True)
    (image / 'lib/systemd/system').mkdir(parents=True)
    for mode in ('y', 'n', 'invalid', ''):
        env = dict(os.environ, ENABLE_REDIS_WRITE_PROTECTION=mode,
                   IMAGE_CONFIGS=str(root / 'files/image_config'), FILESYSTEM_ROOT=str(image),
                   FILESYSTEM_ROOT_USR_LIB_SYSTEMD_SYSTEM=str(image / 'lib/systemd/system'))
        result = subprocess.run(['bash', '-ec', 'sudo() { "$@"; }\n' + script], env=env,
                                capture_output=True, text=True)
        if mode not in ('y', 'n'):
            assert result.returncode != 0
            continue
        assert result.returncode == 0, result.stderr
        with mock.patch.object(startup, 'BUILD_SETTING', image / 'usr/share/sonic/redis-auth/build-enabled'):
            assert startup.build_enabled() is (mode == 'y')
