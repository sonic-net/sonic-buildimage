"""Check the manifest and release labels emitted for a SONiC image."""

import json
import os


def assert_manifest_labels(test, path, manifest):
    labels = {}
    for line in path.read_text().removesuffix("\n").split("\n"):
        key, separator, value = line.partition("=")
        test.assertEqual(separator, "=", f"{path}: malformed label {line!r}")
        test.assertNotIn(key, labels, f"{path}: duplicate label {key!r}")
        labels[key] = value
    test.assertEqual(set(labels), {"com.azure.sonic.manifest", "Tag"}, str(path))
    test.assertEqual(json.loads(labels["com.azure.sonic.manifest"]), manifest, str(path))
    test.assertEqual(labels["Tag"], os.environ["EXPECTED_SONIC_IMAGE_VERSION"], str(path))
