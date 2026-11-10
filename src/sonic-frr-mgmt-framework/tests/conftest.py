"""Keep one daemon module identity across temporary platform-module mocks.

Otherwise patch.dict(sys.modules) removes a newly imported daemon but leaves its
parent-package attribute behind; later string patches target a stale module.
"""
import importlib
import sys
from unittest.mock import patch
from .test_config import mockmapping
import frrcfgd

with patch.dict(sys.modules, mockmapping):
    daemon_module = importlib.import_module('frrcfgd.frrcfgd')
sys.modules['frrcfgd.frrcfgd'] = daemon_module

frrcfgd.frrcfgd = daemon_module
