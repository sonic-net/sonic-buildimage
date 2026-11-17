"""Exercise transfer of native worker APT policy to disposable BuildKit secrets."""

import os
from pathlib import Path
import shlex
import stat
import sys
import tempfile
import unittest
from unittest.mock import patch


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ci"))
import native_docker_apt as apt


class NativeDockerAptTest(unittest.TestCase):
    def setUp(self):
        self.config = {
            "http_detect": "/run/sonic-download-proxy/native_apt_proxy.py",
            "https_detect": "/opt/sonic-native-bin/native-apt-proxy",
            "ca": "/run/sonic-native-apt/apt-ca-bundle.pem",
        }
        self.endpoint = "http://squid.example:3128"

    def test_both_manager_generations_and_scoped_trust(self):
        calls = []

        def query(router, url):
            calls.append((router, url))
            return self.endpoint

        lines, ca = apt.routes(self.config, query)
        self.assertEqual(calls, [(self.config[f"{s}_detect"], f"{s}://deb.debian.org/debian/") for s in ("http", "https")])
        self.assertEqual(lines, [f'Acquire::{s}::Proxy::deb.debian.org "{self.endpoint}";' for s in ("http", "https")]
                         + ['Acquire::https::deb.debian.org::CaInfo "/run/secrets/sonic_apt_ca";'])
        self.assertEqual(ca, self.config["ca"])

    def test_explicit_policy_and_job_bypass(self):
        for key in ("http_detect", "http_proxy", "http_host", "http_legacy"):
            with self.subTest(key=key):
                self.assertEqual(apt.routes(self.config | {key: "DIRECT"}, self.fail)[0], [])
        for key in ("https_detect", "https_proxy", "https_host", "https_legacy", "ca", "global_ca"):
            with self.subTest(key=key):
                self.assertEqual(apt.routes(self.config | {key: "/custom"}, self.fail)[0], [])
        for declined in ("", "DIRECT"):
            self.assertEqual(apt.routes(self.config, lambda *_: declined)[0], [])

    def test_http_host_rule_cannot_override_explicit_https_or_bypass(self):
        for declined in ("", "DIRECT"):
            with self.subTest(declined=declined):
                # APT falls back from a host's HTTPS proxy to its HTTP proxy,
                # ahead of a global HTTPS proxy. Leave both schemes alone.
                routes = {"http": self.endpoint, "https": declined}
                query = lambda router, url: routes[url.split(":", 1)[0]]
                self.assertEqual(apt.routes(self.config, query), ([], None))
                routes = {"http": declined, "https": self.endpoint}
                self.assertEqual(apt.routes(self.config, query), ([
                    f'Acquire::https::Proxy::deb.debian.org "{self.endpoint}";',
                    'Acquire::https::deb.debian.org::CaInfo "/run/secrets/sonic_apt_ca";',
                ], self.config["ca"]))

    def test_unusable_or_credential_bearing_endpoint_rejected_without_echo(self):
        for endpoint in ("http://name:secret@proxy:3128", "http://proxy:3128/?secret", "http://proxy:3128/x",
                         "http://proxy:3128#secret", "http://proxy:3128/\nfoo", "http://proxy:0", "http://proxy:65536"):
            with self.subTest(endpoint=endpoint), self.assertRaisesRegex(ValueError, "^Invalid native worker APT proxy"):
                apt.routes(self.config, lambda *_: endpoint)

    def test_real_apt_config_and_router_keep_secrets_outside_build_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checkout = root / "checkout"
            checkout.mkdir()
            detector = root / "route"
            detector.write_text("#!/bin/sh\n[ -n \"$no_proxy\" ] || echo " + self.endpoint + "\n")
            detector.chmod(0o755)
            ca = root / "bundle"
            ca.touch()
            config_file = root / "apt.conf"
            config_file.write_text(
                f'Dir::Etc::Parts "{checkout}";\nDir::Etc::main "/dev/null";\n'
                + "".join(f'Acquire::{s}::Proxy-Auto-Detect "{detector}";\n' for s in ("http", "https"))
                + f'Acquire::https::deb.debian.org::CaInfo "{ca}";\n')
            with (patch.dict(os.environ, {"APT_CONFIG": str(config_file), "no_proxy": ""}),
                  patch.object(apt, "ROUTERS", {str(detector)}), patch.object(apt, "CA_FILES", {str(ca)})):
                config = apt.settings()
                options = apt.build_options(root, checkout, config)
                tokens = shlex.split(options)
                self.assertEqual(tokens[::2], ["--secret", "--secret"])
                self.assertEqual(tokens[3], "id=sonic_apt_ca,src=" + str(ca))
                secret = Path(tokens[1].split(",src=")[1])
                self.assertEqual(stat.S_IMODE(secret.stat().st_mode), 0o600)
                self.assertIn(self.endpoint, secret.read_text())
                self.assertNotIn(self.endpoint, options)
                self.assertEqual(list(checkout.iterdir()), [])
                with self.assertRaisesRegex(ValueError, "separate runner temporary"):
                    apt.build_options(checkout, checkout, config)
                with patch.dict(os.environ, {"no_proxy": "deb.debian.org"}):
                    self.assertEqual(apt.build_options(root, checkout, config), "")


if __name__ == "__main__":
    unittest.main()
