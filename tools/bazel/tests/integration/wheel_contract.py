"""Compare an installed wheel with its owner's existing setup.py contract."""

import configparser
from contextlib import contextmanager
from email.parser import BytesParser
import importlib.metadata
import os
from pathlib import Path
import runpy
import sys
import types
from unittest import mock
import zipfile

from packaging.requirements import Requirement
from packaging.utils import canonicalize_name
import pkg_resources
import setuptools


@contextmanager
def source_context(directory):
    previous_cwd, previous_path = Path.cwd(), sys.path[:]
    try:
        os.chdir(directory)
        sys.path.insert(0, str(directory))
        yield
    finally:
        os.chdir(previous_cwd)
        sys.path[:] = previous_path


def setup_contract(path):
    """Capture setup.py metadata without building or installing the package."""
    path = Path(path).absolute()
    captured = {}

    def distribution(name):
        # Some owners check prerequisites before declaring metadata. Declared
        # third-party dependencies retain their real versions; these SONiC
        # distributions are supplied by source dependencies in Bazel tests.
        project = pkg_resources.Requirement.parse(name).project_name
        try:
            version = importlib.metadata.version(project)
        except importlib.metadata.PackageNotFoundError:
            if canonicalize_name(project) not in {
                "sonic-py-common", "sonic-yang-mgmt", "sonic-yang-models",
            }:
                raise
            version = "1.0"
        return types.SimpleNamespace(version=version)

    with source_context(path.parent), \
            mock.patch.object(setuptools, "setup", side_effect=lambda **kwargs: captured.update(kwargs)), \
            mock.patch.object(pkg_resources, "get_distribution", side_effect=distribution):
        runpy.run_path(str(path), run_name="__wheel_contract__")
    assert captured, f"No setup metadata captured from {path}"
    return captured


def normalized_requirement(text):
    """Compare requirement names, extras, constraints, markers and direct URLs."""
    requirement = Requirement(text)
    return (canonicalize_name(requirement.name),
            tuple(sorted(canonicalize_name(extra) for extra in requirement.extras)),
            str(requirement.specifier), str(requirement.marker) if requirement.marker else "",
            requirement.url)


def expected_sources(contract, root):
    """Resolve the modules and package Python files declared by setup.py."""
    root = Path(root)
    result = {name + ".py": root / (name + ".py") for name in contract.get("py_modules", [])}
    for package in contract.get("packages", []):
        directory = root / package.replace(".", "/")
        sources = list(directory.glob("*.py"))
        assert sources, f"Package has no declared Python files: {package}"
        result.update({str(source.relative_to(root)): source for source in sources})
    return result


def _expected_requirements(contract):
    requirements = {normalized_requirement(value) for value in contract.get("install_requires", [])}
    for extra, values in contract.get("extras_require", {}).items():
        for value in values:
            requirement, separator, marker = value.partition(";")
            marker = f'({marker.strip()}) and extra == "{extra}"' if separator else f'extra == "{extra}"'
            requirements.add(normalized_requirement(requirement + "; " + marker))
    return requirements


def assert_wheel_contract(wheel, setup, contract, installed):
    """Check metadata, exact source/data bytes, licenses and installed commands.

    Runtime imports and command behavior stay in each component's probe. Script
    bodies must match byte-for-byte; only their installer-selected shebang may
    differ from the original source.
    """
    root = Path(setup).parent
    with zipfile.ZipFile(wheel) as archive:
        names = set(archive.namelist())
        sources = expected_sources(contract, root)
        actual_python = {name for name in names if name.endswith(".py") and
                         ".dist-info/" not in name and ".data/" not in name}
        assert actual_python == set(sources), ("Python payload", actual_python, set(sources))
        for member, source in sources.items():
            assert archive.read(member) == source.read_bytes(), f"Wheel source differs: {member}"
            assert (installed.site / member).read_bytes() == source.read_bytes(), f"Installed source differs: {member}"

        metadata_files = [name for name in names if name.endswith(".dist-info/METADATA")]
        assert len(metadata_files) == 1, ("Wheel metadata files", metadata_files)
        metadata_file = metadata_files[0]
        metadata = BytesParser().parsebytes(archive.read(metadata_file))
        assert canonicalize_name(metadata["Name"]) == canonicalize_name(contract["name"]), metadata["Name"]
        assert metadata["Version"] == contract["version"], ("Version", metadata["Version"], contract["version"])
        actual_requires = {normalized_requirement(value) for value in metadata.get_all("Requires-Dist", [])}
        expected_requires = _expected_requirements(contract)
        assert actual_requires == expected_requires, ("Requires-Dist", actual_requires, expected_requires)

        prefix = metadata_file.split(".dist-info/", 1)[0]
        for directory, files in contract.get("data_files", []):
            for name in files:
                relative = Path(directory.lstrip("/")) / Path(name).name
                # bdist_wheel puts absolute data_files at the archive root;
                # relative destinations use the environment's data scheme.
                if Path(directory).is_absolute():
                    member, target = str(relative), installed.site / relative
                else:
                    member, target = prefix + ".data/data/" + str(relative), installed.environment / relative
                assert archive.read(member) == (root / name).read_bytes(), f"Wheel data differs: {member}"
                assert target.read_bytes() == (root / name).read_bytes(), f"Installed data differs: {target}"

        for name in contract.get("scripts", []):
            target = installed.scripts / Path(name).name
            assert os.access(target, os.X_OK), f"Installed script is not executable: {target}"
            body = (root / name).read_bytes().split(b"\n", 1)[1]
            member = prefix + ".data/scripts/" + Path(name).name
            assert archive.read(member).split(b"\n", 1)[1] == body, f"Wheel script differs: {member}"
            assert target.read_bytes().split(b"\n", 1)[1] == body, f"Installed script differs: {target}"

        expected_entries = contract.get("entry_points", {}).get("console_scripts", [])
        if expected_entries:
            entry_file = prefix + ".dist-info/entry_points.txt"
            entries = configparser.ConfigParser()
            entries.read_string(archive.read(entry_file).decode())
            expected = dict(tuple(part.strip() for part in line.split("=", 1)) for line in expected_entries)
            assert dict(entries["console_scripts"]) == expected, ("Console scripts", dict(entries["console_scripts"]), expected)
            for name in expected:
                target = installed.scripts / name
                assert os.access(target, os.X_OK), f"Installed command is not executable: {target}"

        for name in contract.get("license_files", []):
            members = [member for member in names if member.endswith("/licenses/" + name)]
            assert len(members) == 1, ("License files", name, members)
            assert archive.read(members[0]) == (root / name).read_bytes(), f"Wheel license differs: {name}"
