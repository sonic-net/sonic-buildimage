"""Qualify the initial owned-server deployment before creating boot credentials."""
import configparser
import importlib
import json
from pathlib import Path
import sys
import types

import pytest

import redis_auth_startup as startup

PLATFORM_DIRECTORY = startup.platform_directory
ERRORS = (startup.CredentialStateError, OSError, ValueError, KeyError, TypeError, configparser.Error)


@pytest.fixture
def boot(root_directory, monkeypatch):
    monkeypatch.setattr(startup, 'build_enabled', lambda: True)
    intent = root_directory / 'intent.json'
    intent.write_text('{"mode":"enforce"}')
    return intent, root_directory / 'runtime'


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value if isinstance(value, str) else json.dumps(value))


@pytest.mark.parametrize('field,value', [
    ('type', 'unknown'), ('type', 'SpineRouter'), ('type', 'SmartSwitchDPU'),
    ('platform', 'other'), ('switch_type', 'voq'), ('switch_type', 'chassis-packet'),
    ('switch_type', 'dpu'), ('switch_type', 'unknown'), ('subtype', 'SmartSwitch'),
])
def test_unsupported_metadata_never_creates_credentials(boot, field, value):
    intent, runtime = boot
    path = startup.CONFIG / 'config_db.json'
    data = json.loads(path.read_text())
    data['DEVICE_METADATA']['localhost'][field] = value
    write(path, data)
    with pytest.raises(ERRORS):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


@pytest.mark.parametrize('file,value', [
    ('asic.conf', 'NUM_ASIC=0'), ('asic.conf', 'NUM_ASIC=2'), ('asic.conf', 'NUM_ASIC=unknown'),
    ('asic.conf', ''), ('asic.conf', 'NUM_ASIC=1\nNUM_ASIC=2'),
    ('asic.conf', '[other]\nNUM_ASIC=1'), ('platform.json', '{'),
    ('platform.json', {'DPU': {}}), ('platform.json', {'DPUS': {}}),
    ('platform_env.conf', 'supervisor=1'), ('platform_env.conf', 'switch_bmc=1'),
    ('platform_env.conf', 'disaggregated_chassis=1'), ('platform_env.conf', 'supervisor=unknown'),
    ('chassisdb.conf', ''),
])
def test_unsupported_platform_never_creates_credentials(boot, file, value):
    intent, runtime = boot
    write(startup.platform_directory() / file, value)
    with pytest.raises(ERRORS):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


@pytest.mark.parametrize('file', ['asic.conf', 'platform_env.conf', 'platform.json'])
def test_broken_optional_platform_file_is_not_a_default(boot, file):
    intent, runtime = boot
    (startup.platform_directory() / file).symlink_to('missing')
    with pytest.raises(ERRORS):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


@pytest.mark.parametrize('file', ['config_db.json', 'old_config/config_db.json'])
def test_missing_or_malformed_metadata_is_rejected(boot, file):
    intent, runtime = boot
    path = startup.CONFIG / file
    if file.startswith('old_config/'):
        (startup.CONFIG / 'pending_config_migration').touch()
    path.unlink(missing_ok=True)
    for content in (None, '{', '{}'):
        if content is not None:
            write(path, content)
        with pytest.raises(ERRORS):
            startup.prepare_selected(intent_path=intent, runtime=runtime)
        assert not runtime.exists()


@pytest.mark.parametrize('saved', [False, True])
@pytest.mark.parametrize('file', ['config_db0.json', 'database_config0.json',
    'database_configdpu0.json', 'chassisdb_address', 'enable_multidb', 'database_global.json'])
def test_additional_layouts_are_not_implicitly_owned(boot, saved, file):
    intent, runtime = boot
    write(startup.CONFIG / ('old_config' if saved else '') / file, {})
    with pytest.raises(ERRORS):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_namespace_is_rejected_before_password_creation(boot):
    intent, runtime = boot
    write(startup.NETNS / 'asic0', '')
    with pytest.raises(ERRORS):
        startup.prepare_selected(intent_path=intent, runtime=runtime)
    assert not runtime.exists()


def test_saved_metadata_and_published_inventory_define_the_boot(boot):
    intent, runtime = boot
    saved = startup.CONFIG / 'old_config'
    saved.mkdir()
    (startup.CONFIG / 'pending_config_migration').touch()
    metadata = saved / 'config_db.json'
    (startup.CONFIG / 'config_db.json').rename(metadata)
    write(saved / 'database_global.json', {'INCLUDES': [
        {'include': '../../redis/sonic-db/database_config.json'}]})
    write(startup.platform_directory() / 'asic.conf', 'NUM_ASIC=1')
    # A BMC client setting does not expose a local Redis server.
    write(startup.platform_directory() / 'platform_env.conf', 'switch_host=1')
    write(startup.CONFIG / 'bmc.json', {'bmc_addr': '192.0.2.1'})
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['redis', 'redis_bmp']
    secret = (runtime / 'local-system/writer.secret').read_bytes()
    metadata.unlink()
    assert startup.runtime_required(intent_path=intent, runtime=runtime)
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == ['redis', 'redis_bmp']
    assert (runtime / 'local-system/writer.secret').read_bytes() == secret
    assert sorted(path.name for path in (runtime / 'server').iterdir()) == ['instances.json', 'users.acl']


def test_legacy_does_not_discover_topology(boot, monkeypatch):
    intent, runtime = boot
    intent.write_text('{"mode":"legacy"}')
    monkeypatch.setattr(startup, 'platform_directory', lambda: pytest.fail('unexpected discovery'))
    assert startup.prepare_selected(intent_path=intent, runtime=runtime) == []
    assert not startup.runtime_required(intent_path=intent, runtime=runtime)


@pytest.mark.parametrize('identity', ['example', '.', '..', '../other', 'missing', 'conflict'])
def test_platform_identity_uses_file_api_without_redis(tmp_path, monkeypatch, identity):
    def no_database(*_args, **_kwargs):
        pytest.fail('platform discovery must not connect to Redis')
    native = types.ModuleType('swsscommon.swsscommon')
    native.ConfigDBConnector = native.SonicV2Connector = no_database
    monkeypatch.setitem(sys.modules, 'swsscommon', types.ModuleType('swsscommon'))
    monkeypatch.setitem(sys.modules, 'swsscommon.swsscommon', native)
    root = Path(__file__).resolve().parents[4]
    monkeypatch.syspath_prepend(str(root / 'src/sonic-py-common'))
    device = importlib.import_module('sonic_py_common.device_info')
    machine, directory, alias = tmp_path / 'machine.conf', tmp_path / 'device/example', tmp_path / 'platform-alias'
    directory.mkdir(parents=True)
    write(machine, 'onie_platform=' + ('example' if identity == 'conflict' else identity))
    if identity == 'missing':
        machine.unlink()
    if identity == 'conflict':
        alias.symlink_to('other')
    monkeypatch.delenv('PLATFORM', raising=False)
    monkeypatch.setattr(device, 'MACHINE_CONF_PATH', str(machine))
    monkeypatch.setattr(device, 'HOST_DEVICE_PATH', str(directory.parent))
    monkeypatch.setattr(device, 'CONTAINER_PLATFORM_PATH', str(alias))
    if identity == 'example':
        assert PLATFORM_DIRECTORY() == directory
    else:
        with pytest.raises(ERRORS):
            PLATFORM_DIRECTORY()
