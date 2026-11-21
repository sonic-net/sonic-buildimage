"""Recheck the upstream release indexes used by the pinned kernel's module extensions."""

import hashlib
import json
from urllib.parse import urlsplit


PYTHON_EXTENSION = "@@aspect_rules_py+//py/unstable:extension.bzl%python_interpreters"
GO_EXTENSION = "@@rules_go+//go:extensions.bzl%go_sdk"
GO_RELEASES = "https://go.dev/dl/?mode=json&include=all"
GO_VERSION = "1.26.7"
PYTHON_RELEASES = "https://github.com/astral-sh/python-build-standalone/releases/download"

# The pinned extension selected 372 filename/checksum pairs from these three
# SHA256SUMS files. Bind the complete parsed facts and the exact upstream bytes
# together; adding a new eligible interpreter to an old release also invalidates reuse.
PYTHON_FACTS_SHA256 = "3751896aaf34154e2bc88a5b4d1227f24cfc325cb0c05c0909deb4782bdb96be"
PYTHON_CHECKSUMS = {
    f"{PYTHON_RELEASES}/20241002/SHA256SUMS": "e1b2d14f2f1f69165b6e70675fe62bb3849b36c17bbec8b9381ff73fde9bf18d",
    f"{PYTHON_RELEASES}/20251031/SHA256SUMS": "6e62d9f3f4e47f152326d052324c65b73ea2ea4991b1b815f7d71bf6faddb5f6",
    f"{PYTHON_RELEASES}/20260303/SHA256SUMS": "ad1f7355b9779577a4f860d895383d155fb546af9e63392efc07150644229b67",
}


def allow_redirect(source_url, target_url):
    """GitHub serves these release files through its signed asset URLs."""
    target = urlsplit(target_url)
    return (source_url in PYTHON_CHECKSUMS and target.scheme == "https"
            and target.netloc == "release-assets.githubusercontent.com"
            and target.path.startswith("/github-production-release-asset/162334160/"))


def _go_archives(data):
    try:
        releases = json.loads(data)
        release, = (item for item in releases if item["version"] == "go" + GO_VERSION)
        # Match rules_go's go/private/sdk.bzl _parse_versions_json.
        return {item["os"] + "_" + item["arch"]: [item["filename"], item["sha256"]]
                for item in release["files"] if item["kind"] == "archive"}
    except (ValueError, TypeError, KeyError) as error:
        raise ValueError("Invalid upstream Go SDK release index") from error


def validate(facts, fetch_many):
    if not isinstance(facts, dict) or set(facts) != {PYTHON_EXTENSION, GO_EXTENSION}:
        raise ValueError("Unexpected module extension facts in the kernel lock")
    python = json.dumps(facts[PYTHON_EXTENSION], sort_keys=True, separators=(",", ":")).encode()
    if hashlib.sha256(python).hexdigest() != PYTHON_FACTS_SHA256:
        raise ValueError("Kernel Python interpreter facts changed")
    go = facts[GO_EXTENSION]
    if not isinstance(go, dict) or set(go) != {GO_VERSION}:
        raise ValueError("Unexpected Go SDK version in the kernel lock")
    responses = fetch_many([*PYTHON_CHECKSUMS, GO_RELEASES])
    for url, digest in PYTHON_CHECKSUMS.items():
        data = responses.get(url)
        if not isinstance(data, bytes) or hashlib.sha256(data).hexdigest() != digest:
            raise ValueError("Upstream Python interpreter release checksums changed")
    if _go_archives(responses.get(GO_RELEASES)) != go[GO_VERSION]:
        raise ValueError("Upstream Go SDK archive checksums changed")
