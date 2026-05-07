#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for platform_util.py — focused on getplatform_config_db and getonieplatform
"""

import os
import sys
import pytest
from unittest.mock import patch, mock_open, MagicMock

# Mock heavy platform imports
sys.modules['platform_config'] = MagicMock()

import platform_util as pu


class TestGetplatformConfigDb:
    """Tests for getplatform_config_db()"""

    @patch('os.path.isfile', return_value=False)
    def test_returns_empty_when_config_db_missing(self, mock_isfile):
        result = pu.getplatform_config_db()
        assert result == ""

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_returns_platform_name(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = "x86_64-mlnx_msn2700-r0"
        mock_popen.return_value.stdout = mock_stdout

        result = pu.getplatform_config_db()
        assert result == "x86_64-mlnx_msn2700-r0"

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_returns_empty_when_command_returns_empty(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = ""
        mock_popen.return_value.stdout = mock_stdout

        result = pu.getplatform_config_db()
        assert result == ""

    @patch('subprocess.Popen')
    @patch('os.path.isfile', return_value=True)
    def test_strips_whitespace(self, mock_isfile, mock_popen):
        mock_stdout = MagicMock()
        mock_stdout.read.return_value = "  x86_64-ragile-r0  \n"
        mock_popen.return_value.stdout = mock_stdout

        result = pu.getplatform_config_db()
        assert result == "x86_64-ragile-r0"


class TestGetonieplatform:
    """Tests for getonieplatform()"""

    def test_returns_platform_from_machine_conf(self):
        content = "onie_machine=ragile\nonie_platform=x86_64-ragile-r0\nonie_arch=x86_64\n"
        with patch('os.path.isfile', return_value=True), \
             patch("builtins.open", mock_open(read_data=content)):
            result = pu.getonieplatform("/host/machine.conf")
        assert result == "x86_64-ragile-r0"

    def test_returns_none_when_no_onie_platform(self):
        content = "onie_machine=ragile\nonie_arch=x86_64\n"
        with patch('os.path.isfile', return_value=True), \
             patch("builtins.open", mock_open(read_data=content)):
            result = pu.getonieplatform("/host/machine.conf")
        assert result is None

    def test_returns_empty_when_file_missing(self):
        with patch('os.path.isfile', return_value=False):
            result = pu.getonieplatform("/host/machine.conf")
        assert result == ""

    def test_skips_malformed_lines(self):
        content = "badline\nonie_platform=x86_64-ragile-r0\nanother_bad\n"
        with patch('os.path.isfile', return_value=True), \
             patch("builtins.open", mock_open(read_data=content)):
            result = pu.getonieplatform("/host/machine.conf")
        assert result == "x86_64-ragile-r0"


class TestGetplatformName:
    """Tests for getplatform_name()"""

    @patch.object(pu, 'getonieplatform', return_value="x86_64-ragile-r0")
    @patch('os.path.isfile', return_value=True)
    def test_returns_from_machine_conf(self, mock_isfile, mock_onie):
        result = pu.getplatform_name()
        assert result == "x86_64-ragile-r0"

    @patch.object(pu, 'getplatform_config_db', return_value="x86_64-ragile-r0")
    @patch('os.path.isfile', return_value=False)
    def test_falls_back_to_config_db(self, mock_isfile, mock_cfgdb):
        result = pu.getplatform_name()
        assert result == "x86_64-ragile-r0"


class TestByteTostr:
    """Tests for byteTostr()"""

    def test_bytes_to_str(self):
        result = pu.byteTostr(b'\x48\x65\x6c\x6c\x6f')
        assert result == "Hello"

    def test_empty_bytes(self):
        result = pu.byteTostr(b'')
        assert result == ""


class TestInttostr:
    """Tests for inttostr()"""

    def test_basic(self):
        result = pu.inttostr(0x41424344, 4)
        assert len(result) == 4

    def test_single_byte(self):
        result = pu.inttostr(0x41, 1)
        assert len(result) == 1
