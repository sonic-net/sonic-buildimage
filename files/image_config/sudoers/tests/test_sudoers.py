"""Baseline policy invariants; real sudo must validate effective authorization."""

from pathlib import Path
import re
import unittest


SUDOERS = Path(__file__).resolve().parents[1] / "sudoers"

READ_ONLY_COMMANDS = (
    "/bin/cat /var/log/syslog",
    "/bin/cat /var/log/syslog.1 /var/log/syslog",
    "/bin/cat /var/log/syslog.1",
    "/sbin/ip netns identify [0-9]*",
    "/sbin/brctl show",
    '/usr/bin/TSC ""',
    "/usr/bin/chage ^-l [A-Za-z0-9_.-]+$",
    "/usr/bin/dmesg -D",
    "/usr/bin/docker exec snmp cat /etc/snmp/snmpd.conf",
    "/usr/bin/docker exec bgp cat /etc/quagga/bgpd.conf",
    "/usr/bin/docker exec swss md5sum /usr/bin/arp_update",
    "/usr/bin/docker images *",
    "/usr/bin/docker ps *",
    "/usr/bin/docker ps",
    "/usr/bin/lldpctl",
    "/usr/bin/sensors",
    "/usr/bin/systemctl status",
    "/usr/bin/systemctl status *",
    "/usr/bin/tail -F /var/log/syslog",
    "/usr/bin/rvtysh -c show *",
    "/usr/bin/rvtysh -n [0-9]* -c show *",
    "/usr/bin/vtysh -c show version",
    "/usr/bin/vtysh -c show bgp ipv[46] summary json",
    "/usr/bin/vtysh -n [0-9] -c show version",
    "/usr/bin/vtysh -n [0-9] -c show bgp ipv[46] summary json",
    "/usr/local/bin/decode-syseeprom",
    "/usr/local/bin/ipintutil",
    "/usr/local/bin/lldpshow",
    "/usr/local/bin/pcieutil *",
    "/usr/local/bin/psuutil *",
    "/usr/local/bin/sonic-installer list",
    "/usr/local/bin/sfputil show *",
    "/usr/sbin/dmidecode -s system-product-name",
)


class TestSudoers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        policy = re.sub(r"\\\n", "", SUDOERS.read_text(encoding="utf-8"))
        cls.active_lines = [
            " ".join(line.split())
            for line in policy.splitlines()
            if line.strip()
            and (not line.lstrip().startswith("#")
                 or line.lstrip().startswith("#include"))
        ]

    def test_readonly_commands_preserved_without_full_dump(self):
        declarations = [
            line for line in self.active_lines
            if line.startswith("Cmnd_Alias READ_ONLY_CMDS ")
        ]
        self.assertEqual(
            declarations,
            ["Cmnd_Alias READ_ONLY_CMDS = " + ", ".join(READ_ONLY_COMMANDS)],
        )

    def test_no_explicit_dump_grant(self):
        for line in self.active_lines:
            with self.subTest(line=line):
                self.assertNotIn("generate_dump", line)

    def test_administrative_and_other_user_grants_preserved(self):
        grants = [
            line for line in self.active_lines
            if not line.startswith(("Defaults", "Cmnd_Alias ", "#include"))
        ]
        self.assertEqual(grants, [
            "root ALL=(ALL:ALL) ALL",
            "ALL ALL=NOPASSWD: READ_ONLY_CMDS",
            "%sudo ALL=(ALL:ALL) NOPASSWD: ALL",
        ])

    def test_defaults_and_runtime_include_preserved(self):
        settings = [
            line for line in self.active_lines
            if line.startswith(("Defaults", "#include"))
        ]
        self.assertEqual(settings, [
            "Defaults env_reset",
            'Defaults secure_path="/usr/local/sbin:/usr/local/bin:'
            '/usr/sbin:/usr/bin:/sbin:/bin"',
            'Defaults env_keep += "SONIC_CLI_IFACE_MODE"',
            "Defaults lecture = once",
            "Defaults lecture_file = /etc/sudoers.lecture",
            "Defaults!PASSWD_CMDS !syslog",
            "Defaults passwd_timeout=5",
            "#includedir /etc/sudoers.d",
        ])


if __name__ == "__main__":
    unittest.main()
