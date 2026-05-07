#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for sfp.py — focused on getplatform_config_db and getonieplatform
"""

import os
import sys
import pytest
from unittest.mock import patch, mock_open, MagicMock

# Mock platform-specific imports BEFORE importing sfp
sys.modules['sonic_platform_base'] = MagicMock()
sys.modules['sonic_platform_base.sonic_sfp'] = MagicMock()
sys.modules['sonic_platform_base.sonic_sfp.sfputilhelper'] = MagicMock()
sys.modules['sonic_platform_base.sfp_base'] = MagicMock()
sys.modules['sonic_platform_base.sonic_xcvr'] = MagicMock()
sys.modules['sonic_platform_base.sonic_xcvr.sfp_optoe_base'] = MagicMock()

# Add the sfp.py directory to path and create a fake package
sfp_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, sfp_dir)

# Create a fake package so relative imports work
import types
sonic_platform_pkg = types.ModuleType('sonic_platform')
sonic_platform_pkg.__path__ = [sfp_dir]
sonic_platform_pkg.sfp_config = MagicMock()
sys.modules['sonic_platform'] = sonic_platform_pkg
sys.modules['sonic_platform.sfp_config'] = sonic_platform_pkg.sfp_config

# Now import sfp as part of the package
import importlib.util
spec = importlib.util.spec_from_file_location("sonic_platform.sfp", os.path.join(sfp_dir, "sfp.py"),
                                               submodule_search_locations=[])
sfp = importlib.util.module_from_spec(spec)
sys.modules['sonic_platform.sfp'] = sfp
spec.loader.exec_module(sfp)


class TestGetplatformConfigDb:
    """Tests for getplatform_config_db() in sfp.py"""

    @patch('os.path.isfile', return_value=False)
    def test_returns_empty_when_config_db_missing(self, mock_isfile):
        result = sfp.getplatform_config_db()
        assert result == ""

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_returns_platform_name(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = "x86_64-ragile_ra-b6510-32c-r0"
        mock_popen.return_value.stdout = mock_stdout

        result = sfp.getplatform_config_db()
        assert result == "x86_64-ragile_ra-b6510-32c-r0"

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_returns_empty_when_command_returns_empty(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = ""
        mock_popen.return_value.stdout = mock_stdout

        result = sfp.getplatform_config_db()
        assert result == ""

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_strips_whitespace(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = "  x86_64-ragile-r0  \n"
        mock_popen.return_value.stdout = mock_stdout

        result = sfp.getplatform_config_db()
        assert result == "x86_64-ragile-r0"


class TestGetonieplatform:
    """Tests for getonieplatform() in sfp.py"""

    def test_returns_platform_from_machine_conf(self):
        content = "onie_machine=ragile\nonie_platform=x86_64-ragile_ra-b6510-32c-r0\nonie_arch=x86_64\n"
        with patch('os.path.isfile', return_value=True), \
             patch("builtins.open", mock_open(read_data=content)):
            result = sfp.getonieplatform("/host/machine.conf")
        assert result == "x86_64-ragile_ra-b6510-32c-r0"

    def test_returns_none_when_no_onie_platform(self):
        content = "onie_machine=ragile\nonie_arch=x86_64\n"
        with patch('os.path.isfile', return_value=True), \
             patch("builtins.open", mock_open(read_data=content)):
            result = sfp.getonieplatform("/host/machine.conf")
        assert result is None

    def test_returns_empty_when_file_missing(self):
        with patch('os.path.isfile', return_value=False):
            result = sfp.getonieplatform("/host/machine.conf")
        assert result == ""


class TestGetplatformName:
    """Tests for getplatform_name() in sfp.py"""

    @patch.object(sfp, 'getonieplatform', return_value="x86_64-ragile-r0")
    @patch('os.path.isfile', return_value=True)
    def test_returns_from_machine_conf(self, mock_isfile, mock_onie):
        result = sfp.getplatform_name()
        assert result == "x86_64-ragile-r0"

    @patch.object(sfp, 'getplatform_config_db', return_value="x86_64-ragile-r0")
    @patch('os.path.isfile', return_value=False)
    def test_falls_back_to_config_db(self, mock_isfile, mock_cfgdb):
        result = sfp.getplatform_name()
        assert result == "x86_64-ragile-r0"
