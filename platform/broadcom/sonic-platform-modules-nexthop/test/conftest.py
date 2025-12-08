#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Common configuration for all tests.

This file is automatically loaded by pytest and sets up the test environment
before any test modules are imported.
"""

import os
import sys

# Add the paths for nexthop modules imports by tests (relative imports)
common_path = os.path.join(os.path.dirname(__file__), '../common')
sys.path.insert(0, common_path)
common_path = os.path.join(os.path.dirname(__file__), '../common/sonic_platform')
sys.path.insert(0, common_path)
