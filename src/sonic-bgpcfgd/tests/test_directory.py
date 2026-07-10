from unittest.mock import MagicMock, patch
from bgpcfgd.directory import Directory

@patch('bgpcfgd.directory.log_err')
def test_directory(mocked_log_err):
    test_values = {
        "key1": {
            "key1_1": {
                "key1_1_1": "value1_1_1",
                "key1_1_2": "value1_1_2",
                "key1_1_3": "value1_1_3"
            }
        },
        "key2": {
            "value2"
        }
    }

    directory = Directory()

    # Put test values
    directory.put("db_name", "table", "key1", test_values["key1"])
    directory.put("db_name", "table", "key2", test_values["key2"])

    # Test get_path()
    assert directory.get_path("db_name", "table", "") == test_values
    assert directory.get_path("db_name", "table", "key1/key1_1/key1_1_1") == "value1_1_1"
    assert directory.get_path("db_name", "table", "key1/key_nonexist") == None

    # Test path_exist()
    assert directory.path_exist("db_name", "table", "key1/key1_1/key1_1_1")
    assert not directory.path_exist("db_name", "table_nonexist", "")
    assert not directory.path_exist("db_name", "table", "key1/key_nonexist")

    # Test get_slot()
    assert directory.get_slot("db_name", "table") == test_values

    # Test get()
    assert directory.get("db_name", "table", "key2") == test_values["key2"]

    # Test remove()
    directory.remove("db_name", "table", "key2")
    assert not directory.path_exist("db_name", "table", "key2")

    # Test remove() with invalid input
    directory.remove("db_name", "table_nonexist", "key2")
    mocked_log_err.assert_called_with("Directory: Can't remove key 'key2' from slot 'db_name__table_nonexist'. The slot doesn't exist")
    directory.remove("db_name", "table", "key_nonexist")
    mocked_log_err.assert_called_with("Directory: Can't remove key 'key_nonexist' from slot 'db_name__table'. The key doesn't exist")

    # Test remove_slot()
    directory.remove_slot("db_name", "table")
    assert not directory.available("db_name", "table")

    # Test remove_slot() with nonexist table
    directory.remove_slot("db_name", "table_nonexist")
    mocked_log_err.assert_called_with("Directory: Can't remove slot 'db_name__table_nonexist'. The slot doesn't exist")
