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

    @pytest.mark.parametrize("port,use_tcp", [(0, False), (6379, True)])
    @pytest.mark.parametrize("db_name,namespace", [
        ("CONFIG_DB", daemon_base.EMPTY_NAMESPACE),
        ("APPL_DB", "asic0"),
    ])
    def test_db_connect(self, port, use_tcp, db_name, namespace):
        outer, inner = self._make_swss_mock()
        inner.SonicDBConfig.getDbPort.return_value = port
        kwargs = {"namespace": namespace} if namespace else {}
        with mock.patch.dict("sys.modules",
                             {"swsscommon": outer,
                              "swsscommon.swsscommon": inner}):
            connector = daemon_base.db_connect(db_name, **kwargs)
        inner.SonicDBConfig.getDbPort.assert_called_once_with(db_name, namespace)
        inner.DBConnector.assert_called_once_with(
            db_name, daemon_base.REDIS_TIMEOUT_MSECS, use_tcp, namespace)
        assert connector is inner.DBConnector.return_value

    def test_db_connect_remote(self):
        """db_connect_remote uses DBConnector(db_id, host, port, timeout)."""
        outer, inner = self._make_swss_mock()
        with mock.patch.dict("sys.modules",
                             {"swsscommon": outer,
                              "swsscommon.swsscommon": inner}):
            daemon_base.db_connect_remote(6, "redis_bmc", 6379)
        inner.DBConnector.assert_called_once_with(
            6, "redis_bmc", 6379, daemon_base.REDIS_TIMEOUT_MSECS)
        inner.SonicDBConfig.getDbPort.assert_not_called()

    def test_db_connect_remote_default_port(self):
        """db_connect_remote defaults to port 6379."""
        outer, inner = self._make_swss_mock()
        with mock.patch.dict("sys.modules",
                             {"swsscommon": outer,
                              "swsscommon.swsscommon": inner}):
            daemon_base.db_connect_remote(4, "redis_switch_host")
        inner.DBConnector.assert_called_once_with(
            4, "redis_switch_host", 6379, daemon_base.REDIS_TIMEOUT_MSECS)
        inner.SonicDBConfig.getDbPort.assert_not_called()
