import sys
import types
from unittest.mock import MagicMock

# Mock sonic_py_common and device_info
mock_sonic_py_common = types.ModuleType('sonic_py_common')
mock_device_info = types.ModuleType('sonic_py_common.device_info')
mock_device_info.get_hostname = MagicMock(return_value='none')
mock_sonic_py_common.device_info = mock_device_info
sys.modules['sonic_py_common'] = mock_sonic_py_common
sys.modules['sonic_py_common.device_info'] = mock_device_info

# Mock swsscommon (C extension not available in test environment)
mock_swsscommon_inner = MagicMock()
mock_swsscommon_outer = types.ModuleType('swsscommon')
mock_swsscommon_outer.swsscommon = mock_swsscommon_inner
sys.modules['swsscommon'] = mock_swsscommon_outer
sys.modules['swsscommon.swsscommon'] = mock_swsscommon_inner
