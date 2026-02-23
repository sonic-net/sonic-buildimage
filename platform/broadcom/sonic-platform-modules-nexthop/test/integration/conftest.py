#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test configuration for integration tests.

Integration tests require a SONiC build environment and require not/minimal mocks.
"""

import os
import pytest
import sys

from unittest.mock import patch


@pytest.fixture(scope="module", autouse=True)
def patch_dependencies():
    """Sets up real dependencies for all integration tests.

    This fixture is automatically applied to all tests in the integration/ directory.
    It uses module scope, so it won't interfere with unit tests.
    """
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    sonic_platform_common = os.path.join(
        TEST_DIR, "../../../../../src/sonic-platform-common/"
    )
    pddf_base = os.path.join(
        TEST_DIR, "../../../../../platform/pddf/platform-api-pddf-base"
    )

    with patch.object(sys, "path", [sonic_platform_common, pddf_base] + sys.path):
        # Keep the patch active while integration tests are running
        yield
