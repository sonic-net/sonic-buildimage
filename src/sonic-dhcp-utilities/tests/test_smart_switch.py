import pytest
from common_utils import MockConfigDb, dhcprelayd_refresh_dhcrelay_test, dhcprelayd_proceed_with_check_res_test
from dhcp_utilities.dhcprelayd.dhcprelayd import DHCP_SERVER_CHECKER, MID_PLANE_CHECKER

MOCK_CONFIG_DB_PATH_SMART_SWITCH = "tests/test_data/mock_config_db_smart_switch.json"


def test_dhcprelayd_refresh_dhcrelay(mock_swsscommon_dbconnector_init):
    expected_checkers = set(["MidPlaneTableEventChecker"])
    dhcprelayd_refresh_dhcrelay_test(expected_checkers, True, mock_get_config_db_table)


@pytest.mark.parametrize("feature_enabled", [True, False])
@pytest.mark.parametrize("feature_res", [True, False, None])
@pytest.mark.parametrize("dhcp_server_res", [True, False, None])
def test_dhcprelayd_proceed_with_check_res(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init,
                                           feature_enabled, feature_res, dhcp_server_res):
    enabled_checkers = set([DHCP_SERVER_CHECKER, MID_PLANE_CHECKER])
    expected_checkers = set([DHCP_SERVER_CHECKER, MID_PLANE_CHECKER])
    dhcprelayd_proceed_with_check_res_test(enabled_checkers, feature_enabled, feature_res, dhcp_server_res,
                                           None, True, expected_checkers)


def mock_get_config_db_table(table_name):
    mock_config_db = MockConfigDb(MOCK_CONFIG_DB_PATH_SMART_SWITCH)
    return mock_config_db.get_config_db_table(table_name)
