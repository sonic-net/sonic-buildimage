#!/usr/bin/env python3
"""Tests for VOQ system port generation in sonic-cfggen.

These tests import sonic-cfggen directly as a module and call main() with
mocked sys.argv and device_info.is_generate_voq(). This allows us to use
standard unittest.mock.patch() instead of subprocess or file-based mocking.
"""
import json
import os
import sys
import unittest
from io import StringIO
from unittest import mock

# Import the sonic-cfggen script as a module
# We need to add the parent directory to sys.path first
test_dir = os.path.dirname(os.path.abspath(__file__))
sonic_cfggen_dir = os.path.dirname(test_dir)  # Parent of tests/ is src/sonic-config-engine/
sonic_cfggen_path = os.path.join(sonic_cfggen_dir, "sonic-cfggen")

# Add parent dir to path so sonic-cfggen can import its own modules
if sonic_cfggen_dir not in sys.path:
    sys.path.insert(0, sonic_cfggen_dir)

# Now we can import sonic-cfggen's main function
# The script is named 'sonic-cfggen' (with hyphen, no .py extension), which isn't a valid Python identifier
# So we use importlib.machinery.SourceFileLoader to import it
from importlib.machinery import SourceFileLoader

if not os.path.isfile(sonic_cfggen_path):
    raise FileNotFoundError(f"sonic-cfggen script not found at: {sonic_cfggen_path}\ntest_dir={test_dir}\nsonic_cfggen_dir={sonic_cfggen_dir}")

# Load the module using SourceFileLoader (handles files without .py extension)
loader = SourceFileLoader("sonic_cfggen", sonic_cfggen_path)
sonic_cfggen = loader.load_module()
sys.modules['sonic_cfggen'] = sonic_cfggen  # Add to sys.modules to avoid re-import issues


class TestVoqSystemPortGen(unittest.TestCase):
    """Tests for VOQ system port generation.

    Uses mock.patch to mock device_info.is_generate_voq() by importing
    sonic-cfggen directly instead of running it as a subprocess.
    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.voq_port_config = os.path.join(self.test_dir, "voq-test-port-config.ini")

    def _run_sonic_cfggen(self, args, mock_voq_enabled=None):
        """Run sonic-cfggen main() with mocked sys.argv and device_info.is_generate_voq()

        Args:
            args: List of command-line arguments (without 'sonic-cfggen' prefix)
            mock_voq_enabled: If True/False, mock is_generate_voq() to return that value.
                             If None, don't mock (use real implementation)

        Returns:
            The captured stdout output as a string
        """
        # Prepare sys.argv
        full_args = ['sonic-cfggen'] + args

        # Capture stdout
        captured_output = StringIO()

        with mock.patch('sys.argv', full_args):
            with mock.patch('sys.stdout', captured_output):
                if mock_voq_enabled is not None:
                    # Mock device_info.is_generate_voq() in the sonic_cfggen module namespace
                    with mock.patch('sonic_py_common.device_info.is_generate_voq', return_value=mock_voq_enabled):
                        sonic_cfggen.main()
                else:
                    sonic_cfggen.main()

        return captured_output.getvalue()

    def test_system_port_generated_with_voq_and_preset(self):
        """is_generate_voq()=True (mocked) + --preset -> SYSTEM_PORT generated."""
        args = [
            "-k", "Test-VOQ-SKU",
            "-p", self.voq_port_config,
            "--preset", "l2",
        ]
        output = self._run_sonic_cfggen(args, mock_voq_enabled=True)
        result = json.loads(output)

        self.assertIn(
            "SYSTEM_PORT", result,
            "SYSTEM_PORT should be generated when is_generate_voq()=True and preset is provided",
        )
        self.assertIn("DEVICE_METADATA", result)
        localhost = result["DEVICE_METADATA"]["localhost"]
        self.assertEqual(localhost.get("switch_type"), "voq")
        self.assertIn("switch_id", localhost)
        self.assertIn("max_cores", localhost)

    def test_system_port_not_generated_without_preset(self):
        """is_generate_voq()=True but no --preset -> SYSTEM_PORT NOT generated."""
        args = [
            "-k", "Test-VOQ-SKU",
            "-p", self.voq_port_config,
            "--print-data",
        ]
        output = self._run_sonic_cfggen(args, mock_voq_enabled=True)
        result = json.loads(output)

        self.assertNotIn(
            "SYSTEM_PORT", result,
            "SYSTEM_PORT should NOT be generated when preset is not provided",
        )

    def test_system_port_not_generated_when_not_voq(self):
        """is_generate_voq()=False (mocked) + --preset -> SYSTEM_PORT NOT generated."""
        args = [
            "-k", "Test-SKU",
            "-p", self.voq_port_config,
            "--preset", "l2",
        ]
        output = self._run_sonic_cfggen(args, mock_voq_enabled=False)
        result = json.loads(output)

        self.assertNotIn(
            "SYSTEM_PORT", result,
            "SYSTEM_PORT should NOT be generated when is_generate_voq()=False",
        )

    def test_system_port_not_generated_when_no_mock(self):
        """No mock (real is_generate_voq()) -> SYSTEM_PORT NOT generated (no platform_env.conf)."""
        # Don't mock at all - use real is_generate_voq() which will return False
        args = [
            "-k", "Test-SKU",
            "-p", self.voq_port_config,
            "--preset", "l2",
        ]
        output = self._run_sonic_cfggen(args, mock_voq_enabled=None)
        result = json.loads(output)

        self.assertNotIn(
            "SYSTEM_PORT", result,
            "SYSTEM_PORT should NOT be generated when is_generate_voq() returns False",
        )

    def test_system_port_structure_correctness(self):
        """Verify SYSTEM_PORT structure when generated."""
        args = [
            "-k", "Test-VOQ-SKU",
            "-p", self.voq_port_config,
            "--preset", "l2",
        ]
        output = self._run_sonic_cfggen(args, mock_voq_enabled=True)
        result = json.loads(output)

        self.assertIn("SYSTEM_PORT", result)
        system_ports = result["SYSTEM_PORT"]
        self.assertIsInstance(system_ports, dict)

        for port_name, port_config in system_ports.items():
            self.assertIn(
                "|", port_name,
                f"System port name should contain '|': {port_name}",
            )
            self.assertIn(
                "system_port_id", port_config,
                f"Missing system_port_id in {port_name}",
            )
            self.assertIn(
                "switch_id", port_config,
                f"Missing switch_id in {port_name}",
            )


if __name__ == "__main__":
    unittest.main()
