"""Tests for deterministic gNOI module generation."""

from pathlib import Path
import shutil
import subprocess
import sys
from zipfile import ZipFile


PROJECT_ROOT = Path(__file__).parents[1]


def _copy_generated_free_source(destination):
    def ignore(_path, names):
        ignored = {
            name
            for name in names
            if name.endswith(("_pb2.py", "_pb2_grpc.py", ".egg-info"))
        }
        ignored.update({".eggs", ".pytest_cache", "__pycache__", "build", "dist"})
        return ignored

    shutil.copytree(PROJECT_ROOT, destination, ignore=ignore)


def _build_generated_modules(source, output):
    subprocess.run(
        [sys.executable, "-m", "build", "--wheel", "--outdir", str(output)],
        cwd=source,
        check=True,
    )
    wheel = next(output.glob("*.whl"))
    with ZipFile(wheel) as archive:
        names = sorted(
            name
            for name in archive.namelist()
            if name.startswith("sonic_grpc/gnoi/") and "_pb2" in name
        )
        contents = {name: archive.read(name) for name in names}
        modes = {name: archive.getinfo(name).external_attr >> 16 for name in names}
    return contents, modes


def test_generation_is_deterministic(tmp_path):
    first_source = tmp_path / "first-source"
    second_source = tmp_path / "second-source"
    _copy_generated_free_source(first_source)
    _copy_generated_free_source(second_source)

    first_contents, first_modes = _build_generated_modules(
        first_source, tmp_path / "first-wheel"
    )
    second_contents, second_modes = _build_generated_modules(
        second_source, tmp_path / "second-wheel"
    )

    assert first_contents == second_contents
    assert len(first_contents) == 8
    assert first_modes == second_modes
    assert all(mode & 0o777 == 0o644 for mode in first_modes.values())
