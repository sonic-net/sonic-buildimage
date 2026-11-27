#!/usr/bin/env python3
"""Keep Make patches identical to the selected shared libteam source recipe."""

import ast
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
LIBTEAM = ROOT / "src/libteam"


def make_series():
    lines = (LIBTEAM / "patch/series").read_text().splitlines()
    return [line.strip() for line in lines if line.strip() and not line.lstrip().startswith("#")]


# Ordered patches from the selected sonic-net/sonic-bazel-registry recipe.
# Update only together with the declared owner version and parity review.
RECIPE = ROOT / "tools/bazel/tests/fixtures/libteam-owner-source.json"


def bazel_patches():
    return list(json.loads(RECIPE.read_text())["patches"])


class LibteamPatchSeriesTest(unittest.TestCase):
    def test_bazel_applies_make_series_in_order(self):
        published = bazel_patches()
        series = make_series()
        self.assertEqual(published, series)
        tree = ast.parse((ROOT / "MODULE.bazel").read_text())
        selected = []
        overrides = []
        for node in ast.walk(tree):
            if not isinstance(node, ast.Call):
                continue
            keywords = {item.arg: item.value for item in node.keywords}
            name = getattr(node.func, "id", "")
            if name == "bazel_dep" and ast.literal_eval(keywords["name"]) == "libteam":
                selected.append(ast.literal_eval(keywords["version"]))
            if name.endswith("_override") and ast.literal_eval(keywords["module_name"]) == "libteam":
                overrides.append(name)
        self.assertEqual(selected, [json.loads(RECIPE.read_text())["version"]])
        self.assertEqual(overrides, [])

    def test_every_series_patch_exists(self):
        expected = json.loads(RECIPE.with_name("libteam-owner-patch-bodies.json").read_text())
        for name in make_series():
            path = LIBTEAM / "patch" / name
            self.assertTrue(path.is_file(), name)
            data = path.read_bytes()
            # The shared owner adds purpose/provenance headers. The applied
            # patch body must remain exactly Make's existing patch bytes.
            body = data[data.index(b"diff --git "):]
            self.assertEqual(expected[name], hashlib.sha256(body).hexdigest(), name)


if __name__ == "__main__":
    unittest.main()
