import pytest
from unittest import mock

import os
import json
import sys

import mock_tables

TEST_DATA_PATH = os.path.dirname(os.path.abspath(__file__))

@pytest.fixture()
def mock_db():
    db = mock.Mock()

    with open(os.path.join(TEST_DATA_PATH, "mock_config_db.json")) as f:
        s = f.read()
        mock_config_db = json.loads(s)
    with open(os.path.join(TEST_DATA_PATH, "mock_state_db.json")) as f:
        s = f.read()
        mock_state_db = json.loads(s)

    def keys(table, pattern="*"):
        assert table == "CONFIG_DB" or table == "STATE_DB"

        import fnmatch
        import re

        regex = fnmatch.translate(pattern)
        regex = re.compile(regex)

        if table == "CONFIG_DB":
            return [key for key in mock_config_db if regex.match(key)]
        if table == "STATE_DB":
            return [key for key in mock_state_db if regex.match(key)]

    def get_all(table, key):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            return mock_config_db.get(key, {})
        if table == "STATE_DB":
            return mock_state_db.get(key, {})

    def get(table, key, entry):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            return mock_config_db.get(key, {}).get(entry, None)
        if table == "STATE_DB":
            return mock_state_db.get(key, {}).get(entry, None)

    def hmset(table, key, value):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            mock_config_db[key] = value
        if table == "STATE_DB":
            mock_state_db[key] = value

    def exists(table, key):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            return key in mock_config_db
        if table == "STATE_DB":
            return key in mock_state_db

    def delete(table, key):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            del mock_config_db[key]
        if table == "STATE_DB":
            del mock_state_db[key]

    def set_(table, key, k, v):
        assert table == "CONFIG_DB" or table == "STATE_DB"
        if table == "CONFIG_DB":
            mock_config_db[key][k] = v
        if table == "STATE_DB":
            mock_state_db[key][k] = v

    db.keys = mock.Mock(side_effect=keys)
    db.get_all = mock.Mock(side_effect=get_all)
    db.get = mock.Mock(side_effect=get)
    db.hmset = mock.Mock(side_effect=hmset)
    db.exists = mock.Mock(side_effect=exists)
    db.delete = mock.Mock(side_effect=delete)
    db.set = mock.Mock(side_effect=set_)

    yield db
