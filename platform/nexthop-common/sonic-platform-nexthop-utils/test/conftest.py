#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Common configuration for all nexthop_utils tests.

This file is automatically loaded by pytest and sets up the test environment
before any test modules are imported.
"""

import os
import sys

# Add the parent directory to the Python path to allow importing nexthop_utils
parent_path = os.path.join(os.path.dirname(__file__), "..")
sys.path.insert(0, parent_path)

# Add the test directory to the Python path for fixtures
test_path = os.path.dirname(__file__)
sys.path.insert(0, test_path)
