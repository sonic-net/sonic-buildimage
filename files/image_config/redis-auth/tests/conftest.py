"""Use repository database templates without installing files on the test host."""
import json
import os
from pathlib import Path
import sys
import tempfile

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_startup as startup


@pytest.fixture(autouse=True)
def database_template(monkeypatch, tmp_path):
    root = Path(__file__).resolve().parents[4]
    config = tmp_path / 'sonic'
    config.mkdir()
    platform = tmp_path / 'platform'
    platform.mkdir()
    (config / 'config_db.json').write_text(json.dumps({'DEVICE_METADATA': {'localhost': {
        'platform': platform.name, 'type': 'ToRRouter'}}}))
    template = tmp_path / 'database_config.json.j2'
    template.write_text((root / 'dockers/docker-database/database_config.json.j2').read_text())
    monkeypatch.setattr(startup, 'CONFIG', config)
    monkeypatch.setattr(startup, 'NETNS', tmp_path / 'netns')
    monkeypatch.setattr(startup, 'platform_directory', lambda: platform)
    monkeypatch.setattr(startup, 'DATABASE_TEMPLATE', template)


@pytest.fixture
def root_directory():
    if os.geteuid() != 0:
        pytest.skip('requires root in an isolated container')
    with tempfile.TemporaryDirectory(prefix='redis-auth-test-') as temporary:
        root = Path(temporary)
        root.chmod(0o755)
        yield root
