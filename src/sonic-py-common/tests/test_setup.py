import os
import runpy

import pkg_resources
import pytest
import setuptools


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


@pytest.mark.parametrize("architecture", ["armv6l", "armv7l", "armv8l"])
def test_configured_armhf_alias_excludes_sonic_grpc(monkeypatch, architecture):
    setup_args = {}
    monkeypatch.setenv("CONFIGURED_ARCH", architecture)
    monkeypatch.setattr(pkg_resources, "get_distribution", lambda _name: object())
    monkeypatch.setattr(setuptools, "setup", lambda **kwargs: setup_args.update(kwargs))

    runpy.run_path(os.path.join(PROJECT_ROOT, "setup.py"), run_name="__main__")

    assert "sonic_grpc" not in setup_args["packages"]
    assert not any(
        dependency.startswith(("grpcio", "protobuf"))
        for dependency in setup_args["install_requires"]
    )
