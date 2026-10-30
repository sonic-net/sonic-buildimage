"""Preserve only the nonsecret mode through SONiC first-boot migration."""
import configparser
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_startup as startup
from redis_auth_prepare import CredentialStateError

IMAGE_CONFIG = Path(__file__).resolve().parents[2]
RC_LOCAL = IMAGE_CONFIG / 'platform/rc.local'


def restore(config, prefix='', pending=True):
    if pending:
        (config / 'pending_config_migration').touch()
    unit = configparser.ConfigParser()
    unit.read_string((IMAGE_CONFIG / 'redis-auth/redis-auth-prepare.service').read_text().replace('\\\n', ' '))
    command = shlex.split(unit['Service']['ExecStartPre'])
    assert command[:2] == ['/bin/sh', '-c'] and len(command) == 3
    script = command[2].replace('/etc/sonic', shlex.quote(str(config)))
    script = script.replace('/usr/share/sonic/redis-auth/build-enabled',
                            shlex.quote(str(config / 'build-enabled')))
    return subprocess.run(command[:2] + [prefix + script], capture_output=True, text=True)


def migrate(config, host, prefix=''):
    source = RC_LOCAL.read_text()
    start = source.index('    if [ -d /host/old_config ]; then')
    end = source.index('    elif [ -f /host/minigraph.xml ]; then', start)
    script = (source[start:end] + '    fi\n').replace('/etc/sonic', shlex.quote(str(config)))
    script = script.replace('/host', shlex.quote(str(host)))
    return subprocess.run(['bash', '-ec', prefix + script], capture_output=True, text=True)


@pytest.fixture
def migration(tmp_path):
    config, host = tmp_path / 'etc/sonic', tmp_path / 'host'
    config.mkdir(parents=True)
    original = host / 'old_config/redis-auth.json'
    original.parent.mkdir(parents=True)
    original.write_text('{"mode":"enforce"}')
    original.chmod(0o600)
    return config, host, original


@pytest.mark.parametrize('mode,selected', [('enforce', ['redis', 'redis_bmp']),
                                         ('legacy', [])])
def test_restore_preserves_explicit_mode_and_permissions(tmp_path, mode, selected):
    config = tmp_path / 'config with spaces'
    backup = config / 'old_config'
    backup.mkdir(parents=True)
    original = backup / 'redis-auth.json'
    content = json.dumps({'mode': mode}) + '\n'
    original.write_text(content)
    original.chmod(0o600)
    assert restore(config).returncode == 0
    restored = config / 'redis-auth.json'
    assert restored.read_text() == content
    assert restored.stat().st_uid == 0
    assert restored.stat().st_mode & 0o777 == 0o600
    assert startup.selection(restored)[0] == selected
    assert not (config / 'writer.secret').exists()


def test_absent_backup_does_not_change_existing_selection(tmp_path):
    (tmp_path / 'old_config').mkdir()
    target = tmp_path / 'redis-auth.json'
    target.write_text('{"mode":"legacy"}')
    before = target.stat()
    assert restore(tmp_path).returncode == 0
    assert target.read_text() == '{"mode":"legacy"}'
    assert target.stat().st_ino == before.st_ino


def test_unsafe_mode_stays_rejected_after_copy(tmp_path):
    backup = tmp_path / 'old_config'
    backup.mkdir()
    original = backup / 'redis-auth.json'
    original.write_text('{"mode":"enforce"}')
    original.chmod(0o666)
    assert restore(tmp_path).returncode == 0
    with pytest.raises(CredentialStateError, match='unsafe'):
        startup.selection(tmp_path / 'redis-auth.json')[0]


def test_symlink_stays_rejected_after_copy(tmp_path):
    backup = tmp_path / 'old_config'
    backup.mkdir()
    source = tmp_path / 'target.json'
    source.write_text('{"mode":"enforce"}')
    (backup / 'redis-auth.json').symlink_to(source)
    assert restore(tmp_path).returncode == 0
    assert (tmp_path / 'redis-auth.json').is_symlink()
    with pytest.raises(OSError):
        startup.selection(tmp_path / 'redis-auth.json')[0]


def test_copy_failure_stops_restoration(tmp_path):
    backup = tmp_path / 'old_config'
    backup.mkdir()
    (backup / 'redis-auth.json').write_text('{"mode":"enforce"}')
    assert restore(tmp_path, 'cp() { return 7; }\n').returncode == 7
    assert not (tmp_path / 'redis-auth.json').exists()


def test_build_disabled_skips_restoration_failure(tmp_path):
    backup = tmp_path / 'old_config'
    backup.mkdir()
    (backup / 'redis-auth.json').write_text('{"mode":"enforce"}')
    (tmp_path / 'build-enabled').write_text('n\n')
    assert restore(tmp_path, 'cp() { return 7; }\n').returncode == 0
    assert not (tmp_path / 'redis-auth.json').exists()


def test_first_boot_restore_and_preparation_order():
    script = RC_LOCAL.read_text()
    first_boot = script.index('if [ -f $FIRST_BOOT_FILE ]; then')
    moved = script.index('mv -f /host/old_config /etc/sonic/ || exit 1', first_boot)
    migration = script.index('touch /etc/sonic/pending_config_migration', moved)
    assert moved < migration
    assert 'restore_redis_auth_intent' not in script
    unit = configparser.ConfigParser()
    unit.read_string((IMAGE_CONFIG / 'redis-auth/redis-auth-prepare.service').read_text().replace('\\\n', ' '))
    assert 'rc-local.service' in unit['Unit']['After'].split()
    assert 'rc-local.service' in unit['Unit']['Requires'].split()
    assert 'database.service' in unit['Unit']['Before'].split()


def test_failed_backup_move_stops_before_preparation(migration):
    config, host, original = migration
    backup = original.parent
    result = migrate(config, host, 'mv() { return 7; }\n')
    assert result.returncode == 1
    assert (backup / 'redis-auth.json').exists()
    assert not (config / 'redis-auth.json').exists()
    assert not (config / 'pending_config_migration').exists()


def test_first_boot_marks_migration_before_preparation_restores_policy(migration):
    config, host, original = migration
    (config / 'redis-auth.json').write_text('{"mode":"legacy"}')
    result = migrate(config, host)
    assert result.returncode == 0, result.stderr
    assert (config / 'redis-auth.json').read_text() == '{"mode":"legacy"}'
    assert restore(config, pending=False).returncode == 0
    assert startup.selection(config / 'redis-auth.json')[0] == ['redis', 'redis_bmp']
    assert (config / 'pending_config_migration').exists()
    assert not original.exists()


def test_untrusted_owner_stays_rejected_after_copy(tmp_path):
    original = tmp_path / 'old_config/redis-auth.json'
    original.parent.mkdir()
    original.write_text('{"mode":"enforce"}')
    original.chmod(0o600)
    os.chown(original, 65534, 65534)
    assert restore(tmp_path).returncode == 0
    with pytest.raises(CredentialStateError, match='unsafe'):
        startup.selection(tmp_path / 'redis-auth.json')[0]


def test_failed_copy_retries_moved_backup_without_rerunning_first_boot(migration):
    config, host, original = migration
    first_boot = migrate(config, host)
    assert first_boot.returncode == 0
    assert not original.exists()
    assert (config / 'old_config/redis-auth.json').exists()
    first = restore(config, 'cp() { return 7; }\n', pending=False)
    assert first.returncode == 7
    assert not (config / 'redis-auth.json').exists()
    assert (config / 'pending_config_migration').exists()
    second = restore(config, pending=False)
    assert second.returncode == 0, second.stderr
    assert startup.selection(config / 'redis-auth.json')[0] == ['redis', 'redis_bmp']


def test_completed_migration_does_not_restore_old_selection(tmp_path):
    backup = tmp_path / 'old_config/redis-auth.json'
    backup.parent.mkdir()
    backup.write_text('{"mode":"enforce"}')
    target = tmp_path / 'redis-auth.json'
    target.write_text('{"mode":"legacy"}')
    before = target.stat()
    assert restore(tmp_path, pending=False).returncode == 0
    assert target.read_text() == '{"mode":"legacy"}'
    assert target.stat().st_ino == before.st_ino


def test_real_directory_copy_failure_preserves_backup_and_allows_retry(tmp_path):
    original = tmp_path / 'old_config/redis-auth.json'
    original.parent.mkdir()
    content = '{"mode":"enforce"}'
    original.write_text(content)
    original.chmod(0o600)
    target = tmp_path / 'redis-auth.json'
    target.mkdir(mode=0o700)
    failed = restore(tmp_path)
    assert failed.returncode != 0
    assert 'cannot overwrite directory' in failed.stderr
    assert target.is_dir() and not list(target.iterdir())
    assert original.read_text() == content
    assert (tmp_path / 'pending_config_migration').is_file()
    target.rmdir()
    assert restore(tmp_path, pending=False).returncode == 0
    assert startup.selection(target)[0] == ['redis', 'redis_bmp']


def test_dangling_backup_symlink_cannot_enable_legacy_mode(tmp_path):
    original = tmp_path / 'old_config/redis-auth.json'
    original.parent.mkdir()
    original.symlink_to(tmp_path / 'missing')
    assert restore(tmp_path).returncode == 0
    assert (tmp_path / 'redis-auth.json').is_symlink()
    with pytest.raises(OSError):
        startup.selection(tmp_path / 'redis-auth.json')[0]
