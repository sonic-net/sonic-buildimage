"""Build and atomically export the AMD64 VS installer device-data TAR."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel import build_helpers
from tools.bazel.build_container_batch import archive_outputs


TARGET = "//src/sonic-device-data:vs_runtime_tar"
WORKSPACE = ROOT / "src/sonic-device-data/workspace"
PLATFORM = "//:linux_amd64"


def build(destination):
    cache = os.environ.get("BAZEL_CONTAINER_CACHE_DIR") or os.environ.get("BAZEL_SWSS_CACHE_DIR")
    arguments = os.environ.get("BAZEL_CONTAINER_ARGS") or os.environ.get("BAZEL_SWSS_ARGS", "")
    options = build_helpers.cache_options(cache) + shlex.split(arguments) + build_helpers.remote_cache_options()
    bazel = os.environ.get("BAZEL", "bazel")
    if os.sep in bazel:
        bazel = str(Path(bazel).resolve())
    with TemporaryDirectory(prefix="sonic-device-data-") as temporary:
        events = Path(temporary) / "events.json"
        subprocess.run([
            bazel, "build", *options, "--package_path=%workspace%:" + str(ROOT),
            "--platforms=" + PLATFORM, "--host_platform=" + PLATFORM,
            "--remote_download_outputs=toplevel", "--build_event_json_file=" + str(events),
            "--nobuild_event_json_file_path_conversion",
            "--experimental_build_event_output_group_mode=default=inline_only", TARGET,
        ], cwd=WORKSPACE, check=True)
        build_helpers.export_archive(archive_outputs(events, {TARGET})[TARGET], destination)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    build(parser.parse_args().destination)
