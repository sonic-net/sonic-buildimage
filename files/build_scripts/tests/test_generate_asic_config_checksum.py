import hashlib
import importlib.util
from pathlib import Path


SCRIPT_PATH = Path(__file__).parents[1] / 'generate_asic_config_checksum.py'
SPEC = importlib.util.spec_from_file_location(
    'generate_asic_config_checksum', SCRIPT_PATH)
CHECKSUM_MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKSUM_MODULE)


def test_main_writes_compatible_checksums(tmp_path, monkeypatch):
    first_file = tmp_path / 'first'
    second_file = tmp_path / 'second'
    first_file.write_bytes(b'a' * (CHECKSUM_MODULE.CHUNK_SIZE + 1))
    second_file.write_bytes(b'second file')
    content = first_file.read_bytes() + second_file.read_bytes()

    legacy_output = tmp_path / 'asic_config_checksum'
    sha256_output = tmp_path / 'asic_config_checksum.sha256'
    monkeypatch.setattr(
        CHECKSUM_MODULE,
        'CONFIG_FILES',
        {str(tmp_path): [second_file.name, first_file.name]})
    monkeypatch.setattr(CHECKSUM_MODULE, 'OUTPUT_FILE', str(legacy_output))
    monkeypatch.setattr(
        CHECKSUM_MODULE, 'SHA256_OUTPUT_FILE', str(sha256_output))

    CHECKSUM_MODULE.main()

    assert legacy_output.read_text() == (
        '536657710a292a202f2333199069a83fd26ec331\n')
    assert sha256_output.read_text() == hashlib.sha256(content).hexdigest() + '\n'


def test_generate_checksum_returns_sha256(tmp_path):
    checksum_file = tmp_path / 'config'
    checksum_file.write_bytes(b'ASIC configuration')

    checksum = CHECKSUM_MODULE.generate_checksum([str(checksum_file)])

    assert checksum == hashlib.sha256(checksum_file.read_bytes()).hexdigest()
