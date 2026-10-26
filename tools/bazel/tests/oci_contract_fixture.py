"""Small, unrelated declarations for shared OCI contract and validator tests.

Production image policies are accepted only as explicit test arguments. Fixture
policies deliberately use a third image and a single source package, so shared
helpers cannot pass by knowing Orchagent, Syncd or a three-library inventory.
"""

import argparse
import json
from pathlib import Path


def package_policy(*, image="docker-example", architecture="amd64", retained=True):
    """Describe an unrelated image whose optional retained client needs FIPS."""
    return {
        "schema": 1, "image": image, "architecture": architecture,
        "distribution": "trixie", "retained_source": "make" if retained else "none",
        "features": {"sample_feature": "enabled"},
        "debug_tools": ["gdb", "gdbserver", "sshpass", "strace", "vim"],
        "retained_requirements": ([{"package": "openssh-client", "version_contains": "+fips", "debug": "inherit", "runtime_elf_paths": ["usr/bin/ssh"]}]
                                  if retained else []),
        "native": {"required_packages": [], "required_sonames": {}, "gap_packages": ["openssh-client"],
                   "local_dependency_pattern": "libexample", "imported_symbol_packages": ["libexample-dbgsym"],
                   "required_imported_symbol_paths": [], "require_source": True,
                   "require_imported_symbols": True},
        "image_validation": {},
    }


def source_contract(*, image="docker-example", architecture="amd64"):
    """One source package with independent module, version, target and install paths."""
    commit = "a" * 40
    return {
        "schema": 1, "image": image, "architecture": architecture, "distribution": "trixie",
        "packages": [{
            "package": "libexample", "version": "2.4.0", "architecture": architecture,
            "control_fields": {"Package": "libexample", "Version": "2.4.0", "Architecture": architecture,
                               "Depends": "libc6", "Pre-Depends": "", "Provides": "sample-abi", "Multi-Arch": "same"},
            "source": {"module": "example-owner", "version": "0.2.0-" + commit, "commit": commit,
                       "target": "@example_owner//runtime:archive"},
            "required_paths": {"usr/lib/libexample.so.2.4.0": "elf", "usr/lib/libexample.so.2": "symlink"},
        }],
    }


def owner_policies(argv=None):
    """Read BUILD-generated owner policies without importing owner Python code."""
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--swss-policy", type=Path)
    parser.add_argument("--syncd-policy", type=Path)
    args, remaining = parser.parse_known_args(argv)
    policies = {}
    for name, path in (("swss", args.swss_policy), ("syncd", args.syncd_policy)):
        if path is not None:
            policies[name] = json.loads(path.read_text())
    return policies, remaining


def identity(value):
    return {name: value[name] for name in ("image", "architecture", "distribution")}
