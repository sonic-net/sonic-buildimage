"""Native Redis bindings are mocked; platform I/O uses file-backed fixtures."""
import sys
from unittest.mock import MagicMock


swsscommon = MagicMock()
sys.modules["swsscommon"] = MagicMock(swsscommon=swsscommon)
sys.modules["swsscommon.swsscommon"] = swsscommon
