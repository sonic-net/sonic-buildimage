"""Local conftest: shim sonic_py_common and swsscommon for hosts that
don't have them installed."""
import sys
import types
from unittest.mock import MagicMock


def _install_sonic_py_common_shim():
    if "sonic_py_common" in sys.modules and getattr(
            sys.modules["sonic_py_common"], "__path__", None) is not None:
        return
    spc = types.ModuleType("sonic_py_common")
    spc.__path__ = []  # mark as package so submodule imports work
    sys.modules["sonic_py_common"] = spc

    di = types.ModuleType("sonic_py_common.device_info")
    di.is_chassis = MagicMock(return_value=False)
    di.is_supervisor = MagicMock(return_value=False)
    di.get_platform = MagicMock(return_value="x86_64-kvm_x86_64-r0")
    di.get_hwsku = MagicMock(return_value="Force10-S6000")
    sys.modules["sonic_py_common.device_info"] = di
    spc.device_info = di

    ma = types.ModuleType("sonic_py_common.multi_asic")
    ma.is_multi_asic = MagicMock(return_value=False)
    sys.modules["sonic_py_common.multi_asic"] = ma
    spc.multi_asic = ma

    gm = types.ModuleType("sonic_py_common.general")
    gm.getstatusoutput_noshell = MagicMock(return_value=(0, ""))
    sys.modules["sonic_py_common.general"] = gm
    spc.general = gm


_install_sonic_py_common_shim()

try:
    import swsscommon  # noqa: F401
except ImportError:
    from tests import swsscommon_test
    sys.modules["swsscommon"] = swsscommon_test
    sys.modules["swsscommon.swsscommon"] = swsscommon_test
