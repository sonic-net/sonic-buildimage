"""Use repository database templates without installing files on the test host."""
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
    template = tmp_path / 'database_config.json.j2'
    template.write_text((root / 'dockers/docker-database/database_config.json.j2').read_text())
    monkeypatch.setattr(startup, 'CONFIG', config)
    monkeypatch.setattr(startup, 'DATABASE_TEMPLATE', template)


@pytest.fixture
def root_directory():
    if os.geteuid() != 0:
        pytest.skip('requires root in an isolated container')
    with tempfile.TemporaryDirectory(prefix='redis-auth-test-') as temporary:
        root = Path(temporary)
        root.chmod(0o755)
        yield root
