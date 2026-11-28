"""Check a Bazel-rendered template and the source-owned cfggen executable.

The generated text protects JSON argument handling, declared includes and build
namespace isolation; a separate failing CLI call must leave no output file."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


GENERATED = Path(sys.argv.pop(1))
CFGGEN = str(Path(sys.argv.pop(1)).resolve())


class CfggenTemplateTest(unittest.TestCase):
    def test_action_preserves_json_strings_including_literal_shell_syntax(self):
        """Keep spaces, quotes and shell-looking characters literal through the template action."""
        lines = GENERATED.read_text().splitlines()
        self.assertEqual(lines[0], 'two words \'single\' "double" $HOME $(ignored) `literal` \\ end')
        self.assertEqual(lines[1], "a b|c")

    def test_action_declares_includes_and_clears_runtime_namespace(self):
        """Require included template output and the build-time namespace default in the generated text."""
        contents = GENERATED.read_text()
        self.assertIn("network=198.51.100.0/24", contents)
        self.assertIn("namespace=none", contents)

    def test_failed_template_does_not_publish_a_generated_file(self):
        """Propagate an invalid-interface rendering error without publishing the requested file."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            template = directory / "invalid.j2"
            template.write_text("{{ interface|validate_interface_name }}")
            output = directory / "config.txt"
            environment = dict(os.environ, PLATFORM="sonic-bazel-build", NAMESPACE_ID="")
            result = subprocess.run(
                [CFGGEN, "-a", '{"interface":"Ethernet0; invalid"}', "-t", str(template) + "," + str(output)],
                env=environment, capture_output=True, text=True, timeout=30,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Invalid interface name", result.stderr)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
