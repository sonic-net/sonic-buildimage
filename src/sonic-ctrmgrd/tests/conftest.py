"""Pytest fixtures and import-time mocks for sonic-ctrmgrd tests.

Some of the production modules under test (`ctrmgr/*.py` and helper scripts)
import packages that are only available on a SONiC device, namely
`sonic_py_common` (a pure-Python helper package) and `swsscommon` (a C
extension). Whenever those packages are missing from the test environment we
substitute lightweight mocks that expose the symbols the production modules
and the tests actually use:

* sonic_py_common.device_info.get_hostname
* sonic_py_common.logger.Logger
* sonic_py_common.general.load_module_from_source
* sonic_py_common.general.getstatusoutput_noshell_pipe
* swsscommon.swsscommon

If `sonic_py_common` is genuinely importable, we leave it alone except for
overriding `device_info.get_hostname` to return the canonical hostname
("none") used by the kube_commands test fixtures.
"""

import importlib.machinery
import importlib.util
import sys
import types
from unittest.mock import MagicMock


def _load_module_from_source(module_name, file_path):
    # Use SourceFileLoader explicitly so this works for files without a .py
    # suffix (e.g. ``ctrmgr/docker-wait-any`` and ``ctrmgr/container``), the
    # same behaviour the real ``sonic_py_common.general.load_module_from_source``
    # provides.
    loader = importlib.machinery.SourceFileLoader(module_name, file_path)
    spec = importlib.util.spec_from_loader(module_name, loader)
    if spec is None:
        raise ImportError("Cannot load module {} from {}".format(module_name, file_path))
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    loader.exec_module(module)
    return module


def _install_sonic_py_common_mocks():
    try:
        import sonic_py_common  # noqa: F401
        real_pkg = True
    except Exception:
        real_pkg = False

    if real_pkg:
        try:
            from sonic_py_common import device_info as _device_info
            _device_info.get_hostname = MagicMock(return_value='none')
        except Exception:
            pass
        return

    pkg = types.ModuleType('sonic_py_common')
    pkg.__path__ = []
    sys.modules['sonic_py_common'] = pkg

    # device_info is a MagicMock so any attribute access (get_platform,
    # is_fast_reboot_enabled, ...) auto-vivifies, matching how tests under
    # tests/ patch sub-attributes such as
    # ``device_info.is_fast_reboot_enabled``. ``get_hostname`` is pinned to
    # the canonical value ``none`` used by kube_commands fixtures.
    device_info = MagicMock(name='sonic_py_common.device_info')
    device_info.get_hostname = MagicMock(return_value='none')
    pkg.device_info = device_info
    sys.modules['sonic_py_common.device_info'] = device_info

    logger_mod = MagicMock(name='sonic_py_common.logger')
    logger_mod.Logger = MagicMock()
    pkg.logger = logger_mod
    sys.modules['sonic_py_common.logger'] = logger_mod

    general = types.ModuleType('sonic_py_common.general')
    general.load_module_from_source = _load_module_from_source
    general.getstatusoutput_noshell_pipe = MagicMock(return_value=([0], ''))
    pkg.general = general
    sys.modules['sonic_py_common.general'] = general


def _install_swsscommon_mocks():
    try:
        from swsscommon import swsscommon  # noqa: F401
        return
    except Exception:
        pass

    swsscommon_pkg = types.ModuleType('swsscommon')
    swsscommon_pkg.__path__ = []
    swsscommon_inner = MagicMock()
    swsscommon_pkg.swsscommon = swsscommon_inner
    sys.modules['swsscommon'] = swsscommon_pkg
    sys.modules['swsscommon.swsscommon'] = swsscommon_inner


_install_sonic_py_common_mocks()
_install_swsscommon_mocks()
