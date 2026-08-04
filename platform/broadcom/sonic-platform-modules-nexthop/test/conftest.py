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

# Adds the '../common' directory to the Python path, to allow tests to import
# python modules from the common directory, such as nexthop and sonic_platform.
common_path = os.path.join(os.path.dirname(__file__), "../common")
sys.path.insert(0, common_path)
