import pathlib
import shutil
import subprocess
import tempfile
import unittest


SUDOERS_PATH = pathlib.Path(__file__).parents[1] / "sudoers"
BUILD_TEMPLATE_PATH = (
    pathlib.Path(__file__).parents[3] / "build_templates"
    / "sonic_debian_extension.j2"
)
SMARTCTL_COMMAND = "/usr/sbin/smartctl -a /dev/sda"


def read_command_alias(sudoers_text, alias_name):
    lines = iter(sudoers_text.splitlines())
    alias_prefix = f"Cmnd_Alias      {alias_name} ="

    for line in lines:
        if line.startswith(alias_prefix):
            alias_lines = [line.partition("=")[2].strip()]
            while alias_lines[-1].endswith("\\"):
                alias_lines[-1] = alias_lines[-1][:-1]
                alias_lines.append(next(lines).strip())
            return [
                command.strip()
                for command in " ".join(alias_lines).split(",")
            ]

    raise ValueError(f"Command alias {alias_name} was not found")


class TestSudoers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sudoers_text = SUDOERS_PATH.read_text(encoding="utf-8")
        cls.read_only_commands = read_command_alias(
            cls.sudoers_text, "READ_ONLY_CMDS"
        )

    def test_sudoers_syntax(self):
        visudo = shutil.which("visudo")
        if visudo is None:
            self.skipTest("visudo is not installed")

        # Do not inspect the test host's unrelated /etc/sudoers.d content.
        sudoers_text = self.sudoers_text.replace(
            "#includedir /etc/sudoers.d", "##includedir /etc/sudoers.d"
        )
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8") as file:
            file.write(sudoers_text)
            file.flush()
            result = subprocess.run(
                [visudo, "-cf", file.name],
                capture_output=True,
                check=False,
                text=True,
            )

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_exact_smartctl_command_is_read_only(self):
        self.assertIn(SMARTCTL_COMMAND, self.read_only_commands)

    def test_no_other_smartctl_command_is_read_only(self):
        smartctl_commands = [
            command for command in self.read_only_commands
            if command.startswith("/usr/sbin/smartctl")
        ]
        self.assertEqual(smartctl_commands, [SMARTCTL_COMMAND])

    def test_unapproved_smartctl_variants_are_not_read_only(self):
        unapproved_commands = (
            "/usr/sbin/smartctl -a /dev/sdb",
            "/usr/sbin/smartctl -a /dev/sda --json",
            "/usr/sbin/smartctl -t long /dev/sda",
            "/usr/sbin/smartctl -i /dev/sda",
            "/usr/sbin/smartctl -a /dev/sda extra",
        )
        for command in unapproved_commands:
            with self.subTest(command=command):
                self.assertNotIn(command, self.read_only_commands)

    def test_smartmontools_is_installed_in_host_image(self):
        build_template = BUILD_TEMPLATE_PATH.read_text(encoding="utf-8")
        self.assertIn("apt-get -y install smartmontools", build_template)


if __name__ == "__main__":
    unittest.main()
