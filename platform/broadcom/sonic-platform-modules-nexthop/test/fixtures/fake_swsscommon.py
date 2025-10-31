#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared fake implementation of swsscommon for unit testings.

The real swsscommon module has external dependencies that are
difficult to meet in a unit testing environment. This fake
implementation provides the minimum functionality of the database
required by the platform modules for unit tests.

Call setup_fake_swsscommon() from a pytest fixture to inject
the fake swsscommon module.
"""

import sys

from unittest.mock import Mock


class FakeDBConnector:
    """Fake implementation of swsscommon.DBConnector for testing."""

    def __init__(self, db_name: str, retry: int):
        self.db_name = db_name


class FakeFieldValuePairs:
    """Fake implementation of swsscommon.FieldValuePairs for testing."""

    def __init__(self, fvs: list[tuple[str, str]]):
        self.dict = dict(fvs)


class FakeTable:
    """Fake implementation of swsscommon.Table for testing."""

    # Global in-memory DB. Shared across all FakeTable instances.
    #
    # Key: db_name, Value: table data
    # Example:
    # {
    #     "STATE_DB": {
    #         "FAN_INFO": {
    #             "Fantray1_1": {"max_speed": "75"}
    #             "Fantray1_2": {"max_speed": "75"}
    #         }
    #     }
    # }
    _global_db = {}

    def __init__(self, db_connector: FakeDBConnector, table_name: str):
        self.db_name = db_connector.db_name
        self.table_name = table_name
        if self.db_name not in self._global_db:
            self._global_db[self.db_name] = {}
        if table_name not in self._global_db[self.db_name]:
            self._global_db[self.db_name][table_name] = {}

    def set(self, key: str, fvp: FakeFieldValuePairs) -> None:
        if key not in self._global_db[self.db_name][self.table_name]:
            self._global_db[self.db_name][self.table_name][key] = {}
        self._global_db[self.db_name][self.table_name][key].update(fvp.dict)

    def get(self, key: str) -> tuple[bool, dict]:
        if key in self._global_db[self.db_name][self.table_name]:
            return True, self._global_db[self.db_name][self.table_name][key]
        else:
            return False, {}


def setup_fake_swsscommon():
    """
    Sets up mock swsscommon module that contains a fake, simpliflied
    implementation of the database using an in-memory dictionary. 
    """
    swsscommon = Mock()
    swsscommon.swsscommon.DBConnector = FakeDBConnector
    swsscommon.swsscommon.FieldValuePairs = FakeFieldValuePairs
    swsscommon.swsscommon.Table = FakeTable
    sys.modules["swsscommon"] = swsscommon
