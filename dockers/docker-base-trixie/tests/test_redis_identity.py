"""Exercise account reservation only inside an opted-in disposable container."""
import os
from pathlib import Path
import pwd
import grp
import subprocess

import pytest

SCRIPT = Path(__file__).resolve().parents[1] / 'reserve-redis-identity.sh'


def command(*args):
    return subprocess.run(args, text=True, capture_output=True)


@pytest.fixture(autouse=True)
def disposable_accounts():
    if os.environ.get('REDIS_IDENTITY_TEST_CONTAINER') != '1':
        pytest.skip('requires an explicitly selected disposable container')
    assert os.geteuid() == 0 and Path('/.dockerenv').is_file()
    for name in ('redis', 'identity-check'):
        if command('getent', 'passwd', name).returncode == 0:
            subprocess.run(['userdel', name], check=True)
        if command('getent', 'group', name).returncode == 0:
            subprocess.run(['groupdel', name], check=True)
    yield


def reserve(uid=400, gid=400):
    return command('sh', str(SCRIPT), str(uid), str(gid))


def test_reservation_is_consistent_and_idempotent():
    assert reserve().returncode == 0
    assert reserve().returncode == 0
    identity = pwd.getpwnam('redis')
    assert (identity.pw_uid, identity.pw_gid, identity.pw_shell) == (400, 400, '/usr/sbin/nologin')
    assert grp.getgrnam('redis').gr_gid == 400


def test_configured_uid_and_gid_are_used():
    assert reserve(401, 402).returncode == 0
    identity = pwd.getpwnam('redis')
    assert (identity.pw_uid, identity.pw_gid) == (401, 402)
    assert grp.getgrnam('redis').gr_gid == 402


def test_zero_padded_ids_are_idempotent():
    assert reserve('0401', '0402').returncode == 0
    assert reserve('0401', '0402').returncode == 0
    identity = pwd.getpwnam('redis')
    assert (identity.pw_uid, identity.pw_gid) == (401, 402)
    assert grp.getgrnam('redis').gr_gid == 402


@pytest.mark.parametrize('uid,gid,field', [('', 402, 'UID'), (401, '4x2', 'GID')])
def test_invalid_ids_are_rejected_before_creating_accounts(uid, gid, field):
    result = reserve(uid, gid)
    assert result.returncode != 0 and f'{field} must be a decimal integer' in result.stderr
    assert command('getent', 'passwd', 'redis').returncode != 0
    assert command('getent', 'group', 'redis').returncode != 0


def test_missing_build_arguments_are_rejected():
    result = command('sh', str(SCRIPT))
    assert result.returncode != 0 and 'configured UID and GID' in result.stderr
    assert command('getent', 'passwd', 'redis').returncode != 0


def test_other_account_with_reserved_uid_is_rejected():
    subprocess.run(['useradd', '-M', '-u', '400', '-g', '65534', 'identity-check'], check=True)
    result = reserve()
    assert result.returncode != 0 and 'reserved UID' in result.stderr
    assert command('getent', 'passwd', 'redis').returncode != 0


def test_other_group_with_reserved_gid_is_rejected():
    subprocess.run(['groupadd', '-g', '400', 'identity-check'], check=True)
    result = reserve()
    assert result.returncode != 0 and 'reserved GID' in result.stderr


def test_existing_redis_uid_is_not_reassigned():
    subprocess.run(['groupadd', '-g', '400', 'redis'], check=True)
    subprocess.run(['useradd', '-M', '-u', '399', '-g', '400', 'redis'], check=True)
    result = reserve()
    assert result.returncode != 0 and 'unexpected UID/GID' in result.stderr
    assert pwd.getpwnam('redis').pw_uid == 399


def test_existing_redis_gid_is_not_reassigned():
    subprocess.run(['groupadd', '-g', '399', 'redis'], check=True)
    result = reserve()
    assert result.returncode != 0 and 'unexpected GID' in result.stderr
    assert grp.getgrnam('redis').gr_gid == 399


def test_login_capable_redis_account_is_rejected():
    subprocess.run(['groupadd', '-g', '400', 'redis'], check=True)
    subprocess.run(['useradd', '-M', '-u', '400', '-g', '400', '-s', '/bin/sh', 'redis'], check=True)
    result = reserve()
    assert result.returncode != 0 and 'must not permit login' in result.stderr
