#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test configuration for unit tests.

Unit tests run in isolation from the SONiC environment and require full mocks.
"""

import pytest
import sys

# Import common mocks that are safe for all test types
import fixtures.mock_imports_common

@pytest.fixture(scope="session", autouse=True)
def setup_unit_test_mocks():
    """
    Set up unit test mocks automatically for all unit tests.
    This fixture is automatically applied to all tests in the unit/ directory.
    """
    # Only import and set up unit test mocks when actually running unit tests
    from fixtures.mock_imports_unit_tests import setup_sonic_platform_mocks

    # Set up the mocks
    setup_sonic_platform_mocks()

    yield

    # Cleanup is handled automatically by pytest session teardown

from fixtures.fixtures_unit_test import *