import sys
from unittest.mock import MagicMock, call, patch

from . import swsscommon_test

sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.runner import Runner
from bgpcfgd.static_rt_timer import StaticRouteTimer


@patch("bgpcfgd.static_rt_timer.swsscommon.SonicV2Connector")
def test_static_route_timer_uses_unix_socket(mock_connector):
    mock_connector.return_value.APPL_DB = "APPL_DB"

    StaticRouteTimer()

    mock_connector.assert_called_once_with(use_unix_socket_path=True)
    mock_connector.return_value.connect.assert_called_once_with("APPL_DB")


@patch("bgpcfgd.runner.swsscommon.SubscriberStateTable")
@patch("bgpcfgd.runner.swsscommon.DBConnector")
@patch("bgpcfgd.runner.swsscommon.Select")
@patch("bgpcfgd.runner.swsscommon.SonicDBConfig.getDbId")
def test_runner_keeps_only_chassis_database_on_tcp(
    mock_get_db_id, mock_select, mock_connector, mock_subscriber
):
    mock_get_db_id.side_effect = [6, 12]
    mock_subscriber.return_value.getDbConnector.return_value.getDbId.return_value = 6
    mock_subscriber.return_value.getTableName.return_value = "TABLE"

    runner = Runner(MagicMock())
    local_manager = MagicMock()
    local_manager.get_database.return_value = "STATE_DB"
    local_manager.get_table_name.return_value = "TABLE"
    runner.add_manager(local_manager)

    chassis_manager = MagicMock()
    chassis_manager.get_database.return_value = "CHASSIS_APP_DB"
    chassis_manager.get_table_name.return_value = "TABLE"
    runner.add_manager(chassis_manager)

    assert mock_connector.call_args_list == [
        call("STATE_DB", 0, False),
        call("CHASSIS_APP_DB", 0, True, ""),
    ]
