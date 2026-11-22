"""Regression tests for checking the external facts used by the kernel's lock."""

import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.kernel import cache_facts


class KernelCacheFactsTest(unittest.TestCase):
    def setUp(self):
        self.url = next(iter(cache_facts.PYTHON_CHECKSUMS))
        self.facts = {
            cache_facts.PYTHON_EXTENSION: {"release_index": {"interpreter": "abc"}},
            cache_facts.GO_EXTENSION: {cache_facts.GO_VERSION: {"linux_amd64": ["sdk.tar.gz", "c" * 64]}},
        }
        self.release = {
            "version": "go" + cache_facts.GO_VERSION,
            "files": [{"os": "linux", "arch": "amd64", "kind": "archive",
                       "filename": "sdk.tar.gz", "sha256": "c" * 64}],
        }
        python = json.dumps(self.facts[cache_facts.PYTHON_EXTENSION], sort_keys=True, separators=(",", ":")).encode()
        self.sources = {self.url: b"checksums", cache_facts.GO_RELEASES: json.dumps([self.release]).encode()}
        for name, value in (("PYTHON_FACTS_SHA256", hashlib.sha256(python).hexdigest()),
                            ("PYTHON_CHECKSUMS", {self.url: hashlib.sha256(b"checksums").hexdigest()})):
            patcher = patch.object(cache_facts, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def fetch(self, urls):
        self.assertEqual(set(urls), set(self.sources))
        return self.sources

    def test_current_release_matches_and_unrelated_go_releases_are_ignored(self):
        self.sources[cache_facts.GO_RELEASES] = json.dumps([{"version": "go0.1"}, self.release]).encode()
        cache_facts.validate(self.facts, self.fetch)

    def test_root_checks_every_selected_sdk_and_rejects_extra_facts(self):
        releases = []
        for version in cache_facts.ROOT_GO_VERSIONS:
            self.facts[cache_facts.GO_EXTENSION][version] = {"linux_amd64": ["sdk.tar.gz", "c" * 64]}
            releases.append(dict(self.release, version="go" + version))
        self.sources[cache_facts.GO_RELEASES] = json.dumps(releases).encode()
        cache_facts.validate_root(self.facts, self.fetch)
        with self.assertRaisesRegex(ValueError, "Go"):
            cache_facts.validate(self.facts, self.fetch)
        self.sources[cache_facts.GO_RELEASES] = json.dumps(releases[1:]).encode()
        with self.assertRaisesRegex(ValueError, "Go"):
            cache_facts.validate_root(self.facts, self.fetch)
        self.sources[cache_facts.GO_RELEASES] = json.dumps(releases).encode()
        self.facts[cache_facts.GO_EXTENSION]["1.23.0"]["linux_amd64"][1] = "d" * 64
        with self.assertRaisesRegex(ValueError, "Go"):
            cache_facts.validate_root(self.facts, self.fetch)
        self.facts["unknown extension"] = {}
        with self.assertRaisesRegex(ValueError, "Unexpected"):
            cache_facts.validate_root(self.facts, self.fetch)

    def test_changes_to_known_upstream_releases_fail(self):
        self.sources[self.url] = b"new checksum list"
        with self.assertRaisesRegex(ValueError, "Python"):
            cache_facts.validate(self.facts, self.fetch)
        self.sources[self.url] = b"checksums"
        self.release["files"][0]["sha256"] = "f" * 64
        self.sources[cache_facts.GO_RELEASES] = json.dumps([self.release]).encode()
        with self.assertRaisesRegex(ValueError, "Go"):
            cache_facts.validate(self.facts, self.fetch)

    def test_unknown_or_altered_lock_facts_fail(self):
        with self.assertRaisesRegex(ValueError, "Unexpected"):
            cache_facts.validate({}, self.fetch)
        self.facts[cache_facts.PYTHON_EXTENSION] = {}
        with self.assertRaisesRegex(ValueError, "Python"):
            cache_facts.validate(self.facts, self.fetch)

    def test_only_the_selected_github_asset_redirects_are_allowed(self):
        target = "https://release-assets.githubusercontent.com/github-production-release-asset/162334160/file?sig=ok"
        self.assertTrue(cache_facts.allow_redirect(self.url, target))
        self.assertFalse(cache_facts.allow_redirect(cache_facts.GO_RELEASES, target))
        self.assertFalse(cache_facts.allow_redirect(self.url, target.replace("https:", "http:")))
        self.assertFalse(cache_facts.allow_redirect(self.url, target.replace("162334160", "other-repo")))
        self.assertFalse(cache_facts.allow_redirect(self.url, target.replace(".com/", ".example/")))


if __name__ == "__main__":
    unittest.main()
