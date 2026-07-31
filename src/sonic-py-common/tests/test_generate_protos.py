"""Tests for deterministic gNOI module generation."""

import inspect
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
from zipfile import ZipFile

import pytest

import generate_protos


PROJECT_ROOT = Path(__file__).parents[1]


def test_missing_generator_reports_required_version(monkeypatch, tmp_path):
    def missing_generator(_distribution_name):
        raise generate_protos.PackageNotFoundError

    monkeypatch.setattr(generate_protos, "version", missing_generator)

    with pytest.raises(
        RuntimeError,
        match="grpcio-tools 1.66.2 is required; not installed",
    ):
        generate_protos.generate(tmp_path)


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
        [sys.executable, "-m", "build", "-n", "--outdir", str(output)],
        cwd=source,
        check=True,
    )
    sdist = next(output.glob("*.tar.gz"))
    with tarfile.open(sdist) as archive:
        names = archive.getnames()
    assert any(name.endswith("/proto/PROVENANCE") for name in names)
    assert sum(name.endswith(".proto") for name in names) == 4

    sdist_source = output / "sdist-source"
    with tarfile.open(sdist) as archive:
        kwargs = {}
        if "filter" in inspect.signature(archive.extractall).parameters:
            kwargs["filter"] = "data"
        archive.extractall(sdist_source, **kwargs)
    extracted_root = next(path for path in sdist_source.iterdir() if path.is_dir())
    sdist_wheel = output / "sdist-wheel"
    subprocess.run(
        [
            sys.executable,
            "-m",
            "build",
            "-n",
            "--wheel",
            "--outdir",
            str(sdist_wheel),
        ],
        cwd=extracted_root,
        check=True,
    )
    wheel = next(sdist_wheel.glob("*.whl"))
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
    assert all(mode & 0o111 == 0 for mode in first_modes.values())
