#!/usr/bin/env python3
"""Check the standard downloader data."""
from pathlib import Path
import re
import shlex
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit
from tools.bazel.ci import source_mirrors

ROOT = Path(__file__).resolve().parents[3]
CONFIG = "tools/bazel/debian-pool.cfg"


def rewritten_urls(url, data=None):
    # Bazel 8.5.1 UrlRewriter.applyRewriteRules matches the complete URL without
    # its scheme, collecting every matching replacement in configuration order.
    # These data-only rules use the common Java/Python regex subset.
    candidate = url.split("://", 1)[1]
    result = []
    if data is None:
        data = (ROOT / CONFIG).read_bytes()
    for line in data.decode().splitlines():
        if not line or line.startswith("#"):
            continue
        directive, expression, replacement = line.split()
        if directive != "rewrite":
            raise AssertionError("unexpected downloader directive")
        match = re.fullmatch(expression, candidate)
        if match:
            result.append(re.sub(r"\$(\d+)", lambda group: match[int(group[1])], replacement))
    return list(dict.fromkeys(result)) or [url]


class DebianPoolTest(unittest.TestCase):
    def test_real_locked_packages_keep_the_original_as_second_choice(self):
        examples = [
            ("debian", "20260727T143429Z", "main/a/arping/arping_2.25-1_amd64.deb"),
            ("debian", "20260727T143429Z", "main/libn/libnsl/libnsl-dev_1.3.0-3+b3_amd64.deb"),
            ("debian", "20260727T143429Z", "main/libf/libfido2/libfido2-dev_1.15.0-1+b1_armhf.deb"),
            ("debian", "20260727T143429Z", "main/r/readline/libreadline-dev_8.2-6_amd64.deb"),
            ("debian", "20260727T143429Z", "main/p/python-typing-extensions/python3-typing-extensions_4.13.2-1_all.deb"),
            ("debian-security", "20260726T121236Z", "updates/main/libs/libsodium/libsodium-dev_1.0.18-1+deb13u1_arm64.deb"),
        ]
        for archive, timestamp, path in examples:
            original = f"https://snapshot.debian.org/archive/{archive}/{timestamp}/pool/{path}"
            with self.subTest(package=path):
                self.assertEqual(rewritten_urls(original), [f"https://deb.debian.org/{archive}/pool/{path}", original])

    def test_indexes_source_archives_and_unrelated_urls_are_unchanged(self):
        prefix = "https://snapshot.debian.org/archive/debian/20260727T143429Z/"
        urls = [
            prefix + "dists/trixie/main/binary-amd64/Packages.xz",
            prefix + "dists/trixie/Release", prefix + "dists/trixie/InRelease",
            prefix + "pool/main/a/arping/arping_2.25.orig.tar.gz",
            prefix + "pool/main/a/arping/arping_2.25-1.dsc",
            prefix + "pool/main/a/arping/arping_2.25-1_amd64.deb?download=1",
            prefix + "pool/main/a/arping/arping_2.25-1_amd64.deb#fragment",
            prefix.replace("20260727T143429Z", "latest") + "pool/a.deb",
            prefix.replace("20260727T143429Z", "20260727") + "pool/a.deb",
            prefix.replace("snapshot.debian.org", "snapshot.debian.org.example") + "pool/a.deb",
            prefix.replace("snapshot.debian.org", "other.example") + "pool/a.deb",
            "https://deb.debian.org/debian/pool/main/a/arping/arping_2.25-1_amd64.deb",
            "https://snapshot.debian.org/archive/debian-security/20260726T121236Z/dists/trixie-security/main/binary-arm64/Packages.xz",
        ]
        for url in urls:
            with self.subTest(url=url):
                self.assertEqual(rewritten_urls(url), [url])

    def test_configuration_only_changes_payload_transport(self):
        rules = [line.split() for line in (ROOT / CONFIG).read_text().splitlines() if line and not line.startswith("#")]
        self.assertEqual(len(rules), 4)
        for directive, expression, replacement in rules:
            self.assertEqual(directive, "rewrite")
            self.assertTrue(expression.endswith(r"\.deb))"))
            self.assertTrue(replacement.startswith("https://"))
            self.assertIn(urlsplit(replacement).netloc, ("deb.debian.org", "$1"))

    def test_shared_default_keeps_local_override_and_cache_options(self):
        root_rc = (ROOT / ".bazelrc").read_text().splitlines()
        self.assertIn("import %workspace%/tools/bazel/common.bazelrc", root_rc)
        self.assertFalse(any("downloader_config=" in line for line in root_rc))
        lines = (ROOT / "tools/bazel/common.bazelrc").read_text().splitlines()
        option = "common --downloader_config=tools/bazel/debian_mirrors.cfg"
        self.assertEqual(lines.count(option), 1)
        self.assertLess(lines.index(option), lines.index("try-import %workspace%/.bazelrc.user"))
        self.assertFalse(Path(CONFIG).is_absolute())
        self.assertTrue((ROOT / CONFIG).is_file())
        # No cache location is replaced by the shared default.
        for line in lines:
            self.assertFalse(any(token.startswith(("--disk_cache", "--repository_cache", "--distdir"))
                                 for token in shlex.split(line, comments=True)))

    def test_direct_default_tries_mirror_pool_then_snapshot(self):
        data = (ROOT / "tools/bazel/debian_mirrors.cfg").read_bytes()
        examples = [
            ("debian", "20260727T143429Z", "20260727T000818Z", "main/a/arping/arping_2.25-1_amd64.deb"),
            ("debian-security", "20260726T121236Z", "20260727T000824Z", "updates/main/libs/libsodium/libsodium-dev_1.0.18-1+deb13u1_arm64.deb"),
        ]
        for archive, timestamp, mirror_timestamp, path in examples:
            original = f"https://snapshot.debian.org/archive/{archive}/{timestamp}/pool/{path}"
            with self.subTest(package=path):
                self.assertEqual(rewritten_urls(original, data), [
                    f"https://packages.trafficmanager.net/snapshot/{archive}/{mirror_timestamp}/pool/{path}",
                    f"https://deb.debian.org/{archive}/pool/{path}", original])

    def test_ci_keeps_maintained_mirror_priority_and_pool_fallback(self):
        source = (ROOT / "tools/bazel/tests/fixtures/source-mirrors-9697f3db.cfg").read_bytes()
        with tempfile.TemporaryDirectory() as temporary, patch.object(source_mirrors, "download", return_value=source):
            output = Path(temporary) / "downloader.cfg"
            source_mirrors.prepare(output)
            data = output.read_bytes()
        examples = [
            ("debian", "20260727T143429Z", "20260727T000818Z", "main/a/arping/arping_2.25-1_amd64.deb", []),
            ("debian-security", "20260726T121236Z", "20260727T000824Z", "updates/main/c/c-ares/libc-ares-dev_1.34.5-1+deb13u1_arm64.deb", ["https://snapshot.debian.org/file/109d7800bf370139268bea60e2ea7e2699cc3d48"]),
            ("debian-security", "20260726T121236Z", "20260727T000824Z", "updates/main/o/openssl/openssl-provider-legacy_3.5.6-1~deb13u2_arm64.deb", ["https://snapshot.debian.org/file/d5a41e70f8551fb076606ed0b1c0dab7fa9bc32d"]),
            ("debian-security", "20260726T121236Z", "20260727T000824Z", "updates/main/o/openssl/openssl_3.5.6-1~deb13u2_arm64.deb", ["https://snapshot.debian.org/file/4b411e66ee3d3106570389d5d1e022b15c1975a8"]),
        ]
        for archive, timestamp, mirror_timestamp, path, mirrors in examples:
            original = f"https://snapshot.debian.org/archive/{archive}/{timestamp}/pool/{path}"
            with self.subTest(package=path):
                self.assertEqual(rewritten_urls(original, data), [
                    f"https://packages.trafficmanager.net/snapshot/{archive}/{mirror_timestamp}/pool/{path}",
                    f"https://deb.debian.org/{archive}/pool/{path}", original, *mirrors])


if __name__ == "__main__":
    unittest.main()
