#!/usr/bin/env python3
"""Check generated Make OCI bases, Bazel images and Docker exports together.

Two timestamp-varied base layouts and their layered images/exports must preserve
bytes, configuration and whiteouts. Success prints a JSON receipt. This fixture
uses Linux AMD64 metadata even on ARM64 hosts and checks files, not execution."""

import argparse
import gzip
import io
import json
from pathlib import Path
import sys
import tarfile

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from tools.bazel.tests.oci_base_fixture import OVERLAY_FILES, digest, image_fixture, layer_tar, oci_files


def snapshot(layout):
    """Compare file contents by layout-relative path, independent of directory location."""
    return {str(path.relative_to(layout)): path.read_bytes() for path in sorted(layout.rglob("*")) if path.is_file()}


def check_export(path, tag):
    """Check one three-layer Docker export and return its config and uncompressed layers."""
    with tarfile.open(fileobj=io.BytesIO(gzip.decompress(path.read_bytes())), mode="r:") as archive:
        # Preserve the single-image/tag contract expected by Make's Docker loader.
        manifest = json.load(archive.extractfile("manifest.json"))
        assert len(manifest) == 1
        image = manifest[0]
        assert image["RepoTags"] == [tag], "legacy Docker image tag changed"
        assert len(image["Layers"]) == 3, "expected both base layers and the consumer layer"
        config = json.load(archive.extractfile(image["Config"]))
        # The consumer adds one environment value while retaining base settings.
        # This image is AMD64 regardless of the architecture running this check.
        assert config["os"] == "linux" and config["architecture"] == "amd64"
        assert config["config"]["Env"] == ["BASE_FIXTURE=preserved", "CONSUMER_FIXTURE=added"]
        assert config["config"]["Entrypoint"] == ["/fixture-entrypoint"]
        assert config["config"]["WorkingDir"] == "/fixture-workdir"
        assert config["config"]["Labels"]["sonic.oci.fixture"] == "preserved"
        _, base_layers = image_fixture()
        expected_layers = base_layers + [layer_tar(OVERLAY_FILES)]
        stored_layers = [archive.extractfile(name).read() for name in image["Layers"]]
        actual_layers = [gzip.decompress(layer) if layer.startswith(b"\x1f\x8b") else layer for layer in stored_layers]
        # Compare uncompressed bytes and their config digests to catch reordered,
        # rewritten or dropped layers, including the original whiteout entry.
        assert actual_layers == expected_layers, "consumer changed layer contents, whiteouts or ordering"
        assert config["rootfs"]["diff_ids"] == [digest(layer) for layer in actual_layers]
        # Apply this fixture's regular-file whiteouts to check the resulting
        # contents. This is a focused layer check, not a general image extractor.
        files = {}
        for layer in actual_layers:
            with tarfile.open(fileobj=io.BytesIO(layer), mode="r:") as contents:
                for entry in contents:
                    assert entry.isfile(), "fixture must contain only ordinary files"
                    path = Path(entry.name)
                    if path.name.startswith(".wh."):
                        files.pop(str(path.with_name(path.name[4:])), None)
                    else:
                        files[entry.name] = contents.extractfile(entry).read()
        assert files == {"etc/base": b"updated by second layer\n", "etc/second": b"second layer\n", **OVERLAY_FILES}
        return config, actual_layers


def main():
    """Compare both timestamp-varied handoffs and emit evidence after every check passes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--first-layout", type=Path, required=True)
    parser.add_argument("--second-layout", type=Path, required=True)
    parser.add_argument("--first-image", type=Path, required=True)
    parser.add_argument("--second-image", type=Path, required=True)
    parser.add_argument("--first-export", type=Path, required=True)
    parser.add_argument("--second-export", type=Path, required=True)
    args = parser.parse_args()
    expected = oci_files(*image_fixture())
    # Changing the outer Docker-save timestamps must not alter the native OCI
    # files that Make publishes or the directory Bazel assembles from them.
    assert snapshot(args.first_layout) == expected, "Make or Bazel changed original OCI bytes"
    assert snapshot(args.second_layout) == expected, "outer archive timestamps changed OCI bytes"
    assert snapshot(args.first_image) == snapshot(args.second_image), "OCI consumer output is not reproducible"
    # Export tags deliberately differ; the checked configurations and layers
    # must still agree. Container execution is outside this artifact check.
    assert check_export(args.first_export, "oci-consumer-first:latest") == check_export(args.second_export, "oci-consumer-second:latest")
    print(json.dumps({"architecture": "amd64", "make_oci_bytes_preserved": True,
                      "consumer_images_identical": True, "base_layers_preserved": 2,
                      "consumer_layers_added": 1, "whiteouts_and_tags_preserved": True}, sort_keys=True))


if __name__ == "__main__":
    main()
