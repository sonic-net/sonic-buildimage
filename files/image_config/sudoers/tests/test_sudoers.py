import pathlib
import shutil
import subprocess
import tempfile
import unittest


SUDOERS_PATH = pathlib.Path(__file__).parents[1] / "sudoers"
PARITY_COMMANDS = (
    '/usr/bin/TSC ""',
    "/usr/bin/chage ^-l [A-Za-z0-9_.-]+$",
    "/usr/bin/dmesg -D",
    "/usr/bin/docker exec swss md5sum /usr/bin/arp_update",
    "/usr/bin/systemctl status",
    "/usr/bin/systemctl status *",
    "/usr/sbin/dmidecode -s system-product-name",
)
EXCLUDED_COMMANDS = (
    "/usr/local/bin/sonic_installer list",
    "/usr/sbin/smartctl -a /dev/sda",
)


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

    def test_netaaa_parity_commands_are_read_only(self):
        for command in PARITY_COMMANDS:
            with self.subTest(command=command):
                self.assertIn(command, self.read_only_commands)

    def test_excluded_commands_are_not_added(self):
        for command in EXCLUDED_COMMANDS:
            with self.subTest(command=command):
                self.assertNotIn(command, self.read_only_commands)

    def test_rvtysh_remains_limited_to_show_commands(self):
        rvtysh_commands = [
            command for command in self.read_only_commands
            if command.startswith("/usr/bin/rvtysh")
        ]
        self.assertEqual(
            rvtysh_commands,
            [
                "/usr/bin/rvtysh -c show *",
                "/usr/bin/rvtysh -n [0-9]* -c show *",
            ],
        )


if __name__ == "__main__":
    unittest.main()
