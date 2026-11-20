"""Check direct source TARs against original setup.py and isolated CPython imports.

The original wheel tests retain their own installation checks. This consumer
uses source TARs exclusively; its expected payload/metadata comes from setup.py,
original scripts and original YANG build_py rendering semantics.
"""

import argparse
import base64
import configparser
import csv
from email.parser import BytesParser
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

import jinja2
from setuptools.dist import Distribution

from tools.bazel.tests.integration.wheel_contract import (
    _expected_requirements, expected_sources, normalized_requirement, setup_contract, source_context,
)


parser = argparse.ArgumentParser()
parser.add_argument("--package", action="append", required=True)
parser.add_argument("--setup", action="append", required=True)
parser.add_argument("--redis-wheel", type=Path, required=True)
parser.add_argument("--metadata", nargs="+", required=True)
parser.add_argument("--yang-mgmt-dependencies", nargs="+", required=True)
ARGS = parser.parse_args()
PACKAGES = {name: Path(path).absolute() for name, path in (value.split("=", 1) for value in ARGS.package)}
SETUPS = {name: Path(path).absolute() for name, path in (value.split("=", 1) for value in ARGS.setup)}
SITE = "usr/local/lib/python3.13/dist-packages/"


def check_source_package(name, archive_path, setup, destination):
    contract = setup_contract(setup)
    with tarfile.open(archive_path, "r:") as archive:
        members = {item.name.removeprefix("./"): item for item in archive.getmembers() if item.isfile()}
        payload = {path: archive.extractfile(item).read() for path, item in members.items()}
        for item in archive.getmembers():
            assert item.uid == item.gid == 0, (name, item.name, item.uid, item.gid)
            if item.isdir():
                assert item.mode == 0o755, (item.name, item.mode)
        metadata_paths = [path for path in members if path.startswith(SITE) and path.endswith(".dist-info/METADATA")]
        assert len(metadata_paths) == 1, metadata_paths
        metadata = BytesParser().parsebytes(payload[metadata_paths[0]])
        assert metadata["Name"] == contract["name"] and metadata["Version"] == contract["version"]
        assert {normalized_requirement(value) for value in metadata.get_all("Requires-Dist", [])} == _expected_requirements(contract), name
        for field, key in [("Summary", "description"), ("Author", "author"), ("Author-email", "author_email"),
                           ("Maintainer", "maintainer"), ("Maintainer-email", "maintainer_email"),
                           ("Home-page", "url"), ("License", "license"), ("Keywords", "keywords"),
                           ("Requires-Python", "python_requires")]:
            if contract.get(key):
                assert metadata[field] == contract[key], (name, field, metadata[field], contract[key])
        assert metadata.get_all("Classifier", []) == contract.get("classifiers", []), name
        if contract.get("long_description"):
            assert metadata.get_payload().rstrip() == contract["long_description"].rstrip(), name
        assert set(metadata.get_all("Provides-Extra", [])) == set(contract.get("extras_require", {})), name
        expected_python = expected_sources(contract, setup.parent)
        actual_python = {path[len(SITE):] for path in payload if path.startswith(SITE)
                         and path.endswith(".py") and ".dist-info/" not in path}
        assert actual_python == set(expected_python), (name, actual_python ^ set(expected_python))
        # Upstream setuptools finalization owns default LICENSE/AUTHORS
        # discovery. Use that independent pinned backend metadata contract.
        with source_context(setup.parent):
            original = Distribution()
            original.metadata.license_files = contract.get("license_files")
            original.metadata.license_file = contract.get("license_file")
            original._finalize_license_files()
        assert set(metadata.get_all("License-File", [])) == set(original.metadata.license_files), name
        for filename in original.metadata.license_files:
            license_path = metadata_paths[0].rsplit("/", 1)[0] + "/licenses/" + filename
            assert payload[license_path] == (setup.parent / filename).read_bytes(), filename
        for path, source in expected_python.items():
            assert payload[SITE + path] == source.read_bytes(), (name, path)
            assert members[SITE + path].mode == 0o644, (name, path)
        for source in contract.get("scripts", []):
            path = "usr/local/bin/" + Path(source).name
            assert members[path].mode == 0o755, path
            assert payload[path].split(b"\n", 1) == [b"#!/usr/bin/python3", (setup.parent / source).read_bytes().split(b"\n", 1)[1]], path
        entry_points = contract.get("entry_points", {})
        if entry_points:
            parent = metadata_paths[0].rsplit("/", 1)[0]
            entries = configparser.ConfigParser()
            entries.read_string(payload[parent + "/entry_points.txt"].decode())
            for group, values in entry_points.items():
                assert dict(entries[group]) == dict(tuple(part.strip() for part in value.split("=", 1)) for value in values)
                if group == "console_scripts":
                    for command in entries[group]:
                        path = "usr/local/bin/" + command
                        assert members[path].mode == 0o755
                        assert payload[path].startswith(b"#!/usr/bin/python3\n")
        for directory, sources in contract.get("data_files", []):
            if name == "sonic-yang-models":
                # Original build_py renders every template twice and copies
                # static models into the CVL namespace. No setup build runs here.
                expected = {Path(source).name for source in sources}
                prefix = "usr/local/" + directory + "/"
                assert {path[len(prefix):] for path in payload if path.startswith(prefix)} == expected
                env = jinja2.Environment(loader=jinja2.FileSystemLoader(setup.parent / "yang-templates"), trim_blocks=True)
                for filename in expected:
                    template = setup.parent / "yang-templates" / (filename + ".j2")
                    source = setup.parent / "yang-models" / filename
                    expected_bytes = (env.get_template(template.name).render(yang_model_type="py" if directory == "yang-models" else "cvl").encode()
                                      if template.is_file() else source.read_bytes())
                    assert payload[prefix + filename] == expected_bytes, (directory, filename)
            else:
                for source in sources:
                    # Existing absolute setup.py data_files live at wheel root,
                    # hence under installed purelib; retain that public layout.
                    prefix = SITE if Path(directory).is_absolute() else "usr/local/"
                    path = prefix + directory.lstrip("/") + "/" + Path(source).name
                    assert payload[path] == (setup.parent / source).read_bytes(), path
                    assert members[path].mode == 0o644, path
        records = [path for path in payload if path.endswith(".dist-info/RECORD")]
        assert len(records) == 1, (name, records)
        record = records[0]
        seen = set()
        for relative, digest, size in csv.reader(io.StringIO(payload[record].decode())):
            assert relative and not Path(relative).is_absolute(), relative
            path = os.path.normpath(SITE + relative)
            assert path in payload and path not in seen, (record, path)
            seen.add(path)
            if path == record:
                assert digest == size == "", record
            else:
                assert digest and size, path
                algorithm, encoded = digest.split("=", 1)
                actual = base64.urlsafe_b64encode(hashlib.new(algorithm, payload[path]).digest()).rstrip(b"=").decode()
                assert encoded == actual and int(size) == len(payload[path]), path
        assert seen == set(payload), (name, seen ^ set(payload))
        archive.extractall(destination, filter="data")
    return {"name": metadata["Name"], "version": metadata["Version"], "sha256": hashlib.sha256(archive_path.read_bytes()).hexdigest(), "files": len(payload)}


def main():
    # setup.py queries Redis distribution metadata. The isolated install probe
    # below retains only directory entries, so it cannot import from the wheel.
    sys.path.insert(0, str(ARGS.redis_wheel.absolute()))
    with tempfile.TemporaryDirectory(dir=os.environ["TEST_TMPDIR"]) as temporary:
        root = Path(temporary)
        records = [check_source_package(name, archive, SETUPS[name], root) for name, archive in PACKAGES.items()]
        runtime_records = [json.loads(Path(path).read_text()) for path in ARGS.metadata]
        assert {record["distribution"] for record in runtime_records} == set(PACKAGES)
        # rules/sonic-yang-mgmt-py3.mk declares models in RDEPENDS; source
        # PythonRuntimeInfo must retain that installation dependency itself.
        assert {json.loads(Path(path).read_text())["distribution"] for path in ARGS.yang_mgmt_dependencies} == {"sonic-yang-mgmt", "sonic-yang-models"}
        for record in runtime_records:
            contract = setup_contract(SETUPS[record["distribution"]])
            assert record["version"] == contract["version"]
            assert {normalized_requirement(value) for value in record["requires_dist"]} == _expected_requirements(contract)
            assert record["requires_python"] == contract.get("python_requires", "")
            assert record["entry_points"] == contract.get("entry_points", {})
            assert set(record["extras"]) == set(contract.get("extras_require", {}))
        site = root / SITE
        # Retain declared external dependencies while excluding all owner source
        # roots. The installed modules must resolve inside this actual TAR tree.
        own = [Path(__file__).absolute().parents[4], Path(__file__).resolve().parents[4]]
        external = [str(Path(path).resolve()) for path in sys.path if path and Path(path).is_dir()
                    and not any(Path(path).resolve().is_relative_to(parent) for parent in own)]
        bootstrap = "import sys;sys.path[:]=[sys.argv[1]]+" + repr(external) + ";"
        probe = bootstrap + '''
import importlib.metadata as metadata, pathlib, json
import sonic_py_common, sonic_yang, config_samples, containercfgd
site=pathlib.Path(sys.argv[1])
for module in [sonic_py_common, sonic_yang, config_samples, containercfgd]:
 assert pathlib.Path(module.__file__).resolve().is_relative_to(site), module.__file__
for name,version in json.loads(sys.argv[2]).items():
 distribution=metadata.distribution(name)
 assert distribution.version==version
 assert pathlib.Path(distribution.locate_file('')).resolve()==site
print('Installed source-only imports and six distribution identities passed')
'''
        versions = {name: setup_contract(SETUPS[name])["version"] for name in PACKAGES}
        result = subprocess.run([sys.executable, "-I", "-B", "-c", probe, str(site), json.dumps(versions)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        print(result.stdout)
        for command in ["sonic-cfggen", "sonic-cfg-help"]:
            script = bootstrap + "import runpy;sys.argv=[sys.argv[2],'--help'];runpy.run_path(sys.argv[0],run_name='__main__')"
            result = subprocess.run([sys.executable, "-I", "-B", "-c", script, str(site), str(root / "usr/local/bin" / command)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            assert result.returncode == 0, result.stderr
        output = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR")
        if output:
            Path(output, "source-installation.json").write_text(json.dumps({"python": sys.version, "packages": records, "installed_imports": "PASS", "cfggen_cfghelp_help": "PASS"}, indent=2) + "\n")
        print('Verified six source TAR payloads, original metadata, scripts, RECORD, both YANG variants and isolated installed consumers')


if __name__ == "__main__":
    main()
