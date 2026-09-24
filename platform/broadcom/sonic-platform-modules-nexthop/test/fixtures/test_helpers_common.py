#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test helpers for sonic_platform tests.
This file provides common helpers that can be used across all test modules.
"""

import os
import sys
import tempfile

from contextlib import contextmanager
from unittest.mock import Mock
from fixtures.fake_swsscommon import FakeDBConnector, FakeFieldValuePairs, FakeTable


def mock_pddf_data(data: dict[str, dict]):
    """Returns a mock PDDF data object.

    This can be passed to init() of many components in sonic_platform.
    """
    data_mock = Mock()
    data_mock.data = data
    return data_mock


def mock_data_in_swsscommon(
    db_name: str, table_name: str, key_to_fvs: dict[str, dict[str, str]]
):
    """Helper function to add data into the fake swsscommon database.

    For example, calling

    mock_data_in_swsscommon("STATE_DB", "FAN_INFO", {
        "Fantray1_1": {"max_speed": 75, "mock_key": "mock_value"},
        "Fantray1_2": {"max_speed": 75}
    })

    will mock the following data in the database:

    STATE_DB|FAN_INFO|Fantray1_1|max_speed = 75
    STATE_DB|FAN_INFO|Fantray1_1|mock_key = "mock_value"
    STATE_DB|FAN_INFO|Fantray1_2|max_speed = 75
    """
    assert (
        sys.modules["swsscommon"].swsscommon.DBConnector == FakeDBConnector
    ), "fake swsscommon is not patched. Make sure to patch them for mock_data_in_swsscommon() to take effect."

    db_connector = FakeDBConnector(db_name, 0)
    table = FakeTable(db_connector, table_name)
    for key, fvs in key_to_fvs.items():
        fvs = [(k, v) for k, v in fvs.items()]
        table.set(key, FakeFieldValuePairs(fvs))


@contextmanager
def temp_file(content: str | bytes, file_prefix: str = "", dir_prefix: str = "test."):
    """
    Creates a temporary file, under a temporary directory.

    The temporary directory and file are deleted after the context manager exits.

    Args:
        dir_prefix: prefix for the temporary directory.
        content: content to write to the temporary file.

    Returns:
        Path to the created file
    """
    root = tempfile.mkdtemp(prefix=dir_prefix)
    fd, filepath = tempfile.mkstemp(prefix=file_prefix, dir=root)
    try:
        if isinstance(content, str):
            content = content.encode("utf-8")
        os.write(fd, content)
        os.close(fd)

        yield filepath
    finally:
        if os.path.exists(filepath):
            os.remove(filepath)
        try:
            os.rmdir(root)
        except OSError:
            # Directory not empty; leave it
            pass
