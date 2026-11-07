import importlib.util
import time
from pathlib import Path


SCRIPT_PATH = Path(__file__).parents[1] / "sbom_parse_lockfiles.py"
SPEC = importlib.util.spec_from_file_location("sbom_parse_lockfiles", SCRIPT_PATH)
SBOM_PARSE_LOCKFILES = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SBOM_PARSE_LOCKFILES)


def test_parse_pnpm_lock_yaml_packages():
    text = """\
packages:
  /plain@1.2.3:
  /@scope/package@4.5.6:
  /plain@1.2.3:
snapshots:
"""

    components = SBOM_PARSE_LOCKFILES.parse_pnpm_lock_yaml(text, "docker-test")

    assert [(item["name"], item["version"]) for item in components] == [
        ("plain", "1.2.3"),
        ("@scope/package", "4.5.6"),
    ]


def test_parse_pnpm_lock_yaml_long_malformed_key_is_linear():
    text = "packages:\n  /" + ("package/" * 25000) + "missing-version:\n"

    start = time.monotonic()
    components = SBOM_PARSE_LOCKFILES.parse_pnpm_lock_yaml(text, "docker-test")
    elapsed = time.monotonic() - start

    assert components == []
    assert elapsed < 2
