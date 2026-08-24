#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Unit tests for __init__.py module
"""

from mellanox_fw_manager import *
import os
import sys
import unittest
from unittest.mock import patch, MagicMock

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestInitModule(unittest.TestCase):
    """Test cases for __init__.py module"""

    def test_imports(self):
        """Test that all expected imports are available"""
        from mellanox_fw_manager.fw_manager import (
            FirmwareManagerBase,
            SpectrumFirmwareManager,
            BluefieldFirmwareManager,
            FirmwareCoordinator,
            FirmwareManagerError,
            FirmwareUpgradeError,
            FirmwareUpgradePartialError
        )

        from mellanox_fw_manager.main import main

        from mellanox_fw_manager.asic_manager import AsicManager

        self.assertTrue(hasattr(FirmwareManagerBase, '__init__'))
        self.assertTrue(hasattr(SpectrumFirmwareManager, '__init__'))
        self.assertTrue(hasattr(BluefieldFirmwareManager, '__init__'))
        self.assertTrue(hasattr(FirmwareCoordinator, '__init__'))
        self.assertTrue(hasattr(AsicManager, '__init__'))

        self.assertTrue(issubclass(FirmwareManagerError, Exception))
        self.assertTrue(issubclass(FirmwareUpgradeError, Exception))
        self.assertTrue(issubclass(FirmwareUpgradePartialError, Exception))

        self.assertTrue(callable(main))

    def test_module_attributes(self):
        """Test that module has expected attributes"""
        import mellanox_fw_manager

        self.assertTrue(hasattr(mellanox_fw_manager, '__version__'))
        self.assertEqual(mellanox_fw_manager.__version__, "1.0.0")

        self.assertTrue(hasattr(mellanox_fw_manager, '__author__'))
        self.assertEqual(mellanox_fw_manager.__author__, "SONiC Team")

    def test_import_error_handling(self):
        """Test that import errors are handled gracefully"""
        try:
            import mellanox_fw_manager
            self.assertTrue(True)
        except ImportError as e:
            self.fail(f"Import failed: {e}")

    def test_all_imports_available(self):
        """Test that all expected symbols are available in the module"""
        import mellanox_fw_manager

        expected_symbols = [
            'FirmwareManagerBase',
            'SpectrumFirmwareManager',
            'BluefieldFirmwareManager',
            'FirmwareManagerError',
            'FirmwareUpgradeError',
            'FirmwareUpgradePartialError',
            'AsicManager',
            '__version__',
            '__author__'
        ]

        for symbol in expected_symbols:
            self.assertTrue(hasattr(mellanox_fw_manager, symbol), f"Symbol {symbol} not found in __init__.py")


if __name__ == '__main__':
    unittest.main()
