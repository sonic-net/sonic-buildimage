#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Configuration for integration tests.

Integration tests require a SONiC build environment and require not/minimal mocks.
"""

import os
import sys
import pytest

# Add the test directory to Python path for imports
test_root = os.path.join(os.path.dirname(__file__), '..')
sys.path.insert(0, test_root)

# Add the common directory to path for nexthop modules imports by tests
common_path = os.path.join(test_root, '../common')
sys.path.insert(0, common_path)

# Mock out common imports. This is done by importing the following module
import fixtures.mock_imports_common