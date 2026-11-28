"""Exercise timeout programming without opening a hardware watchdog."""

import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "watchdog-control.sh"


class WatchdogControlTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.sysfs = Path(self.temp.name)
        (self.sysfs / "timeout").write_text("600\n")
        (self.sysfs / "pretimeout").write_text("300\n")
        (self.sysfs / "options").write_text("0x83a0\n")
        self.log = self.sysfs / "calls"

    def run_script(self, config, action="validate_watchdog_settings"):
        shell = r'''
source "$TEST_SCRIPT"
debug() { :; }
select_watchdog_device() { echo /dev/watchdog-test; }
get_watchdog_sysfs_path() { echo "$TEST_SYSFS"; }
systemctl() { echo "systemctl $*" >> "$TEST_LOG"; }
wdctl() {
    echo "wdctl $*" >> "$TEST_LOG"
    [[ "${TEST_FAIL_WDCTL:-0}" == 0 ]] || return 1
    while (( $# > 1 )); do
        case "$1" in
            --settimeout) echo "$2" > "$TEST_SYSFS/timeout"; shift 2 ;;
            --setpretimeout) echo "$2" > "$TEST_SYSFS/pretimeout"; shift 2 ;;
            *) return 1 ;;
        esac
    done
}
WATCHDOG_CONFIG_OVERRIDE="$TEST_CONFIG"
eval "$TEST_ACTION"
'''
        env = dict(os.environ, PLATFORM="unit-test", TEST_SCRIPT=str(SCRIPT),
                   TEST_SYSFS=str(self.sysfs), TEST_LOG=str(self.log),
                   TEST_CONFIG=json.dumps(config), TEST_ACTION=action)
        return subprocess.run(["bash", "-c", shell], env=env, text=True, capture_output=True)

    def pair(self):
        return tuple(int((self.sysfs / name).read_text()) for name in ("timeout", "pretimeout"))

    def program(self, config):
        return self.run_script(config, '''
validate_watchdog_settings && configure_watchdog_timeouts \
    "$watchdog_device" "$watchdog_timeout" "$watchdog_pretimeout" "$watchdog_sysfs_path"
''')

    def test_defaults_preserve_existing_default_timing(self):
        # Defaults must not be inferred from the current hardware values.
        (self.sysfs / "timeout").write_text("900\n")
        (self.sysfs / "pretimeout").write_text("100\n")
        result = self.program({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.pair(), (600, 300))

    def test_configured_pair_is_programmed_directly(self):
        result = self.program({"timeout": "480", "pretimeout": "300"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.pair(), (480, 300))

    def test_lower_timeout_clears_previous_pretimeout_first(self):
        result = self.program({"timeout": "180", "pretimeout": "60"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.pair(), (180, 60))
        self.assertEqual(self.log.read_text().splitlines(), [
            "wdctl --setpretimeout 0 /dev/watchdog-test",
            "wdctl --settimeout 180 --setpretimeout 60 /dev/watchdog-test",
        ])

    def test_zero_disables_pretimeout(self):
        result = self.program({"timeout": "180", "pretimeout": "0"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.pair(), (180, 0))

    def test_unsupported_device_uses_zero_default_pretimeout(self):
        (self.sysfs / "pretimeout").unlink()
        result = self.program({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.sysfs / "pretimeout").exists())
        self.assertEqual(self.log.read_text().strip(), "wdctl --settimeout 600 /dev/watchdog-test")

    def test_unsupported_device_rejects_nonzero_pretimeout(self):
        (self.sysfs / "pretimeout").unlink()
        result = self.run_script({"timeout": "600", "pretimeout": "1"}, "enable_linux_watchdog_service")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("does not support pretimeout", result.stderr)
        self.assertNotIn("systemctl stop", self.log.read_text())

    def test_visible_attribute_without_capability_uses_zero(self):
        (self.sysfs / "options").write_text("0x8180\n")
        (self.sysfs / "pretimeout").write_text("0\n")
        result = self.program({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.pair(), (600, 0))
        self.assertEqual(self.log.read_text().strip(), "wdctl --settimeout 600 /dev/watchdog-test")

    def test_visible_attribute_without_capability_rejects_nonzero(self):
        (self.sysfs / "options").write_text("0x8180\n")
        result = self.run_script({"timeout": "600", "pretimeout": "1"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("does not support pretimeout", result.stderr)
        self.assertFalse(self.log.exists())

    def test_invalid_pair_does_not_stop_daemon(self):
        for timeout, pretimeout in (("300", "300"), ("300", "301"), ("0", "0"),
                                    ("600", "-1"), ("bad", "0"), ("600", "99999999999")):
            with self.subTest(timeout=timeout, pretimeout=pretimeout):
                if self.log.exists():
                    self.log.unlink()
                result = self.run_script({"timeout": timeout, "pretimeout": pretimeout},
                                         "enable_linux_watchdog_service")
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("systemctl stop", self.log.read_text())
                self.assertEqual(self.pair(), (600, 300))

    def test_hardware_limits_are_checked_before_stop(self):
        (self.sysfs / "max_timeout").write_text("500\n")
        result = self.run_script({"timeout": "600", "pretimeout": "0"}, "enable_linux_watchdog_service")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("max_timeout", result.stderr)
        self.assertNotIn("systemctl stop", self.log.read_text())

    def test_validation_mode_does_not_program_hardware(self):
        result = self.run_script({}, '''main --validate '{"timeout":"480","pretimeout":"300"}' ''')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.log.exists())
        self.assertEqual(self.pair(), (600, 300))


if __name__ == "__main__":
    unittest.main()
