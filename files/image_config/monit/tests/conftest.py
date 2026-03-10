"""
Pytest configuration file for memory_checker tests.

This file sets up the test environment by:
1. Mocking system dependencies (docker, swsscommon) that may not be available in CI
2. Dynamically loading the memory_checker script (which has no .py extension)
"""

import sys
import os
from unittest.mock import MagicMock
import importlib.util
import importlib.machinery

# Mock system dependencies before any imports
sys.modules['docker'] = MagicMock()
sys.modules['swsscommon'] = MagicMock()
sys.modules['swsscommon.swsscommon'] = MagicMock()

# Get the path to the memory_checker script (no .py extension)
memory_checker_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'memory_checker')

# Dynamically load memory_checker as a module
spec = importlib.util.spec_from_loader(
    'memory_checker',
    importlib.machinery.SourceFileLoader('memory_checker', memory_checker_path)
)
memory_checker = importlib.util.module_from_spec(spec)
sys.modules['memory_checker'] = memory_checker
spec.loader.exec_module(memory_checker)

