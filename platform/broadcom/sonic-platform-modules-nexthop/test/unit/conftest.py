#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test configuration for unit tests.

Unit tests run in isolation from the SONiC environment and require full mocks.
"""

import pytest
import sys
from unittest.mock import patch


@pytest.fixture(scope="function", autouse=True)
def patch_dependencies():
    """Sets up mocked/faked dependencies for all unit tests.

    This fixture is automatically applied to all tests in the unit/ directory.
    It uses function scope, so each testcase can override the mocked/faked modules if needed.
    """
    from fixtures.mock_imports_unit_tests import dependencies_dict

    with patch.dict(sys.modules, dependencies_dict()):
        # Keep the patch active while a testcase is running
        yield

    # Cleanup is handled automatically by pytest session teardown
