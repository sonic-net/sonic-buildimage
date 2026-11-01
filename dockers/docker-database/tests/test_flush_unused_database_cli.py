"""Execute the readiness probe with a real subprocess and an isolated CLI fixture."""
import json
import os
from pathlib import Path
import runpy
import sys
import types


def test_readiness_passes_socket_option_as_a_separate_argument(tmp_path, monkeypatch):
    record = tmp_path / 'argv.json'
    executable = tmp_path / 'sonic-db-cli'
    executable.write_text(
        '#!/usr/bin/env python3\n'
        'import json, os, sys\n'
        'from pathlib import Path\n'
        'Path(os.environ["CLI_ARG_RECORD"]).write_text(json.dumps(sys.argv[1:]))\n'
        'print("PONG")\n')
    executable.chmod(0o755)
    monkeypatch.setenv('PATH', str(tmp_path) + os.pathsep + os.environ['PATH'])
    monkeypatch.setenv('CLI_ARG_RECORD', str(record))
    package = types.ModuleType('swsscommon')
    package.swsscommon = types.SimpleNamespace(
        SonicDBConfig=types.SimpleNamespace(getInstanceList=lambda: {}))
    monkeypatch.setitem(sys.modules, 'swsscommon', package)
    script = Path(__file__).resolve().parents[1] / 'flush_unused_database'
    runpy.run_path(str(script), run_name='__main__')
    assert json.loads(record.read_text()) == ['-s', 'PING']
