import os
import runpy

import pkg_resources
import pytest
import setuptools


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class Distribution:
    def __init__(self, version):
        self.version = version


@pytest.mark.parametrize("architecture", ["armv6l", "armv7l", "armv8l"])
def test_configured_armhf_alias_excludes_sonic_grpc(monkeypatch, architecture):
    setup_args = {}
    monkeypatch.setenv("CONFIGURED_ARCH", architecture)
    monkeypatch.setattr(
        pkg_resources,
        "get_distribution",
        lambda _name: Distribution("1.0"),
    )
    monkeypatch.setattr(setuptools, "setup", lambda **kwargs: setup_args.update(kwargs))

    runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")

    assert "sonic_grpc" not in setup_args["packages"]
    assert not any(
        dependency.startswith(("grpcio", "protobuf"))
        for dependency in setup_args["install_requires"]
    )


def test_grpc_dependencies_must_be_preinstalled(monkeypatch):
    setup_args = {}
    installed = {
        "redis-dump-load": Distribution("1.0"),
        "grpcio": Distribution("1.71.0"),
        "grpcio-tools": Distribution("1.71.0"),
        "protobuf": Distribution("5.29.6"),
    }
    monkeypatch.setenv("CONFIGURED_ARCH", "amd64")
    monkeypatch.setattr(
        pkg_resources,
        "get_distribution",
        lambda name: installed[name],
    )
    monkeypatch.setattr(setuptools, "setup", lambda **kwargs: setup_args.update(kwargs))

    runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")

    assert "grpcio>=1.71.0" in setup_args["install_requires"]
    assert "protobuf>=5.29.6,<8" in setup_args["install_requires"]
    assert not any(
        dependency.startswith(("grpcio", "protobuf"))
        for dependency in setup_args["setup_requires"]
    )
    assert setup_args["extras_require"]["testing"] == ["pytest"]


def test_missing_grpc_dependency_stops_before_setup(monkeypatch):
    def get_distribution(name):
        if name == "grpcio":
            raise pkg_resources.DistributionNotFound
        return Distribution("1.0")

    monkeypatch.setenv("CONFIGURED_ARCH", "amd64")
    monkeypatch.setattr(pkg_resources, "get_distribution", get_distribution)
    monkeypatch.setattr(
        setuptools,
        "setup",
        lambda **_kwargs: pytest.fail("setup must not resolve missing dependencies"),
    )

    with pytest.raises(SystemExit):
        runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")
