import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

import pytest

from sonic_py_common import daemon_base


class TestDaemonBase:

    def _make_swss_mock(self):
        mock_swsscommon_inner = mock.MagicMock()
        mock_swsscommon_outer = mock.MagicMock()
        mock_swsscommon_outer.swsscommon = mock_swsscommon_inner
        return mock_swsscommon_outer, mock_swsscommon_inner

    def test_db_connect_remote(self):
        """db_connect_remote uses DBConnector(db_id, host, port, timeout)."""
        outer, inner = self._make_swss_mock()
        with mock.patch.dict("sys.modules",
                             {"swsscommon": outer,
                              "swsscommon.swsscommon": inner}):
            daemon_base.db_connect_remote(6, "redis_bmc", 6379)
        inner.DBConnector.assert_called_once_with(
            6, "redis_bmc", 6379, daemon_base.REDIS_TIMEOUT_MSECS)

    def test_db_connect_remote_default_port(self):
        """db_connect_remote defaults to port 6379."""
        outer, inner = self._make_swss_mock()
        with mock.patch.dict("sys.modules",
                             {"swsscommon": outer,
                              "swsscommon.swsscommon": inner}):
            daemon_base.db_connect_remote(4, "redis_switch_host")
        inner.DBConnector.assert_called_once_with(
            4, "redis_switch_host", 6379, daemon_base.REDIS_TIMEOUT_MSECS)
