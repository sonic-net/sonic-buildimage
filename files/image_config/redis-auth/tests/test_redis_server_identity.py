import importlib.util
import json
import os
from pathlib import Path
from types import SimpleNamespace

import pytest

import redis_auth_startup as startup
from redis_auth_prepare import CredentialStateError

ROOT = Path(__file__).resolve().parents[4]
SPEC = importlib.util.spec_from_file_location(
    'identity_configure', ROOT / 'dockers/docker-database/redis_auth_configure.py')
configure = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(configure)


@pytest.fixture(autouse=True)
def service_identity(monkeypatch):
    monkeypatch.setattr(startup.pwd, 'getpwnam',
                        lambda name: SimpleNamespace(pw_uid=400, pw_gid=400))
    monkeypatch.setattr(startup.grp, 'getgrnam', lambda name: SimpleNamespace(gr_gid=400))


@pytest.mark.parametrize('uid,gid', [(400, 400), (401, 402)])
def test_policy_records_reserved_uid(monkeypatch, uid, gid):
    monkeypatch.setattr(startup.pwd, 'getpwnam', lambda name: SimpleNamespace(pw_uid=uid, pw_gid=gid))
    monkeypatch.setattr(startup.grp, 'getgrnam', lambda name: SimpleNamespace(gr_gid=gid))
    instances = {'redis': {'hostname': '127.0.0.1', 'port': 6379,
                           'unix_socket_path': '/var/run/redis/redis.sock', 'protected': True}}
    policy = json.loads(startup.policy_files(['redis'], instances)['local.json'])
    assert policy == [{'username': startup.USERNAME, 'unix_socket': '/var/run/redis/redis.sock',
                       'tcp_host': '127.0.0.1', 'tcp_port': 0, 'server_uid': uid}]


@pytest.mark.parametrize('uid,gid,group', [(400, 399, 400), (400, 400, 399)])
def test_inconsistent_identity_is_rejected(monkeypatch, uid, gid, group):
    monkeypatch.setattr(startup.pwd, 'getpwnam', lambda name: SimpleNamespace(pw_uid=uid, pw_gid=gid))
    monkeypatch.setattr(startup.grp, 'getgrnam', lambda name: SimpleNamespace(gr_gid=group))
    with pytest.raises(CredentialStateError, match='UID/GID'):
        startup.redis_server_uid()


def test_missing_identity_is_rejected(monkeypatch):
    def missing(name):
        raise KeyError(name)
    monkeypatch.setattr(startup.pwd, 'getpwnam', missing)
    with pytest.raises(CredentialStateError, match='missing reserved'):
        startup.redis_server_uid()


@pytest.mark.parametrize('uid', [None, 399, '400'])
def test_container_rejects_a_policy_for_another_identity(uid):
    instances = {'redis': {'hostname': '127.0.0.1', 'port': 6379,
                           'unix_socket_path': '/var/run/redis/redis.sock', 'protected': True}}
    policy = json.loads(startup.policy_files(['redis'], instances)['local.json'])
    policy[0]['server_uid'] = uid
    settings = {key: value for key, value in instances['redis'].items() if key != 'protected'}
    database = {'INSTANCES': {'redis': settings}}
    additional = {'INSTANCES': {'redis': {'is_protected_mode': True}}}
    with pytest.raises(ValueError, match='differs from inventory'):
        configure.configure(database, additional, policy, instances, environ={}, chassis=False)


@pytest.mark.parametrize('owner', [0, 400, 65534])
def test_socket_directory_owner(root_directory, owner):
    directory = root_directory / 'socket-directory'
    directory.mkdir(mode=0o755)
    os.chown(directory, owner, owner)
    if owner in (0, 400):
        configure.check_socket_directory(directory)
    else:
        with pytest.raises(ValueError, match='ownership'):
            configure.check_socket_directory(directory)


@pytest.mark.parametrize('mode', [0o775, 0o777])
def test_socket_directory_write_permissions(root_directory, mode):
    directory = root_directory / 'socket-directory'
    directory.mkdir(mode=mode)
    directory.chmod(mode)
    with pytest.raises(ValueError, match='permissions'):
        configure.check_socket_directory(directory)


def test_socket_directory_link_is_rejected(root_directory):
    target = root_directory / 'directory'
    target.mkdir(mode=0o755)
    link = root_directory / 'link'
    link.symlink_to(target, target_is_directory=True)
    with pytest.raises(ValueError, match='ownership'):
        configure.check_socket_directory(link)
