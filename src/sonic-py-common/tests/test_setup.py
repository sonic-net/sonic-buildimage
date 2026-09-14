import os
import runpy
import sys

import pkg_resources
import pytest
import setuptools


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


@pytest.mark.parametrize("architecture", ["armhf", "armv6l", "armv7l", "armv8l"])
def test_configured_armhf_alias_excludes_sonic_grpc(monkeypatch, architecture):
    setup_args = {}
    monkeypatch.setenv("CONFIGURED_ARCH", architecture)

    def get_distribution(name):
        assert name == "redis-dump-load"
        return pkg_resources.Distribution(version="1.0")

    monkeypatch.setattr(pkg_resources, "get_distribution", get_distribution)
    monkeypatch.setattr(setuptools, "setup", lambda **kwargs: setup_args.update(kwargs))

    runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")

    assert "sonic_grpc" not in setup_args["packages"]
    assert not any(
        dependency.startswith(("grpcio", "protobuf"))
        for dependency in setup_args["install_requires"]
    )


@pytest.mark.skipif(sys.version_info < (3, 9), reason="sonic_grpc requires Python 3.9")
@pytest.mark.parametrize("runtime_version", ["1.71.0", "1.72.0"])
def test_grpc_dependencies_must_be_preinstalled(monkeypatch, runtime_version):
    setup_args = {}
    installed = {
        "redis-dump-load": "1.0",
        "grpcio": runtime_version,
        "grpcio-tools": "1.71.0",
        "protobuf": "5.29.6",
    }
    monkeypatch.setenv("CONFIGURED_ARCH", "amd64")
    monkeypatch.setattr(
        pkg_resources,
        "get_distribution",
        lambda name: pkg_resources.Distribution(version=installed[name]),
    )
    monkeypatch.setattr(setuptools, "setup", lambda **kwargs: setup_args.update(kwargs))

    runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")

    assert "grpcio>=1.71.0" in setup_args["install_requires"]
    assert "protobuf>=5.29.6,<8" in setup_args["install_requires"]
    assert not any(
        dependency.startswith("grpcio-tools")
        for dependency in setup_args["install_requires"]
    )
    assert not any(
        dependency.startswith(("grpcio", "protobuf"))
        for field in ("setup_requires", "tests_require")
        for dependency in setup_args[field]
    )
    assert setup_args["extras_require"]["testing"] == ["pytest"]


@pytest.mark.skipif(sys.version_info < (3, 9), reason="sonic_grpc requires Python 3.9")
@pytest.mark.parametrize("package, installed_version", [
    ("grpcio", None),
    ("grpcio-tools", None),
    ("protobuf", None),
    ("grpcio", "1.66.2"),
    ("grpcio-tools", "1.66.2"),
    ("grpcio-tools", "1.72.0"),
    ("protobuf", "5.29.5"),
    ("protobuf", "8.0.0"),
])
def test_incompatible_dependency_stops_before_setup(monkeypatch, package, installed_version):
    installed = {
        "redis-dump-load": "1.0",
        "grpcio": "1.71.0",
        "grpcio-tools": "1.71.0",
        "protobuf": "5.29.6",
    }
    installed[package] = installed_version

    def get_distribution(name):
        if installed[name] is None:
            raise pkg_resources.DistributionNotFound
        return pkg_resources.Distribution(version=installed[name])

    monkeypatch.setenv("CONFIGURED_ARCH", "amd64")
    monkeypatch.setattr(pkg_resources, "get_distribution", get_distribution)
    monkeypatch.setattr(
        setuptools,
        "setup",
        lambda **_kwargs: pytest.fail("setup must not resolve incompatible dependencies"),
    )

    with pytest.raises(SystemExit):
        runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")
