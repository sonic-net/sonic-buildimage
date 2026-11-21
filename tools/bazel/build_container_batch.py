"""Build selected Make container archives together with each owner's original Bazel settings."""

import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from urllib.parse import unquote, urlsplit

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.bazel import build_helpers
from tools.bazel.ci import time_vs_image


# Bazel applies this after rc/config expansion. Publishing only requires the
# top-level outputs. The policy keeps an existing "all" download mode intact.
EXPORT_POLICY = (
    '--invocation_policy=flag_policies { flag_name: "remote_download_outputs" '
    'disallow_values { disallowed_values: "minimal" new_value: "toplevel" } }'
)
GO_PROXY_TIMEOUT = re.compile(
    r'\+go_deps\+[^:\s]+: fetch_repo: \S+@\S+: Get "https://proxy\.golang\.org/[^"\s]+/@v/[^"\s]+": '
    r'net/http: TLS handshake timeout\Z'
)


def go_proxy_timed_out(events):
    """Require a completed build event stream with only the Gazelle fetch error."""
    timed_out = False
    analysis_failed = False
    last_message = False
    try:
        with events.open() as stream:
            for line in stream:
                event = json.loads(line)
                last_message = event.get("lastMessage") is True
                if "action" in event and not event["action"].get("success"):
                    return False
                if "aborted" in event:
                    failure = event["aborted"]
                    # Bazel also marks unbuilt dependents and unfinished targets.
                    if not failure.get("description") and failure.get("reason") in (
                        "ANALYSIS_FAILURE", "INCOMPLETE",
                    ):
                        continue
                    label = event["id"].get("unconfiguredLabel", {}).get("label", "")
                    if not (failure.get("reason") == "LOADING_FAILURE" and label.startswith("//external:")
                            and GO_PROXY_TIMEOUT.search(failure.get("description", ""))):
                        return False
                    timed_out = True
                if "finished" in event:
                    finished = event["finished"]
                    detail = finished.get("failureDetail", {})
                    message = detail.get("message", "")
                    cause = message.rsplit("build aborted: ", 1)[-1]
                    analysis_failed = (finished.get("exitCode", {}).get("code") == 1
                                       and ("analysis" in detail or "packageLoading" in detail)
                                       and (cause == "Analysis failed" or GO_PROXY_TIMEOUT.search(cause) is not None))
    except (OSError, ValueError, KeyError, AttributeError, TypeError):
        return False
    return timed_out and analysis_failed and last_message


def starlark_setting(option):
    key, equals, value = option.partition("=")
    if not (equals and key.startswith(("--//", "--@")) and "//" in key and ":" in key):
        raise ValueError("Unsupported per-container Bazel option: " + option)
    return key[2:], value


def settings_from(arguments, rc):
    """Read explicit flags and the checked-in settings selected by Syncd's profile."""
    values = {}
    for group in arguments:
        for option in shlex.split(group):
            if option == "--config=syncd-dash":
                profile = []
                for line in rc.read_text().splitlines():
                    words = shlex.split(line, comments=True)
                    if words[:1] == ["build:syncd-dash"]:
                        profile.extend(words[1:])
                if not profile:
                    raise ValueError(f"Missing build:syncd-dash in {rc}")
            else:
                profile = [option]
            for flag in profile:
                key, value = starlark_setting(flag)
                if key in values and values[key] != value:
                    raise ValueError(f"Conflicting shared values for {key}: {values[key]!r} and {value!r}")
                values[key] = value
    return values


def build_labels(workspace, archives):
    """Main-repository producer labels already match Bazel's canonical event IDs."""
    labels = archives.copy()
    aliases = {name: label for name, label in archives.items()
               if not (label.startswith(("//", "@@")) and ":" in label)}
    if not aliases:
        return labels
    # BEP rewrites apparent external repo names to canonical Bzlmod names and
    # expands label shorthand. Identity filegroups give these exports stable IDs.
    contents = "".join(
        f'filegroup(name={json.dumps(name)}, srcs=[{json.dumps(label)}], tags=["manual"])\n'
        for name, label in sorted(aliases.items())
    )
    package = Path("target/bazel-container-batch") / hashlib.sha256(contents.encode()).hexdigest()
    directory = workspace / package
    directory.mkdir(parents=True, exist_ok=True)
    definition = directory / "BUILD.bazel"
    if not definition.is_file():
        # Concurrent builds may be preparing the same package.
        with tempfile.NamedTemporaryFile(mode="w", dir=directory, delete=False) as temporary:
            temporary.write(contents)
        Path(temporary.name).replace(definition)
    for name in aliases:
        labels[name] = f"//{package}:{name}"
    return labels


def archive_outputs(events, labels, layout_labels=()):
    """Resolve only the successful top-level targets' single declared file."""
    outputs = {}
    with events.open() as stream:
        for line in stream:
            event = json.loads(line)
            target = event.get("id", {}).get("targetCompleted", {})
            label = target.get("label")
            if label not in labels or target.get("aspect"):
                continue
            completed = event["completed"]
            if not completed.get("success") or label in outputs:
                raise ValueError("Unsuccessful or duplicate Bazel target: " + label)
            files = [item for group in completed.get("outputGroup", []) if group["name"] == "default"
                     for item in group.get("inlineFiles", [])]
            is_layout = label in layout_labels
            if is_layout:
                # TreeArtifacts expand to their members in inline BEP output groups.
                files = [item for item in files if item["name"].endswith("/oci-layout")]
            if len(files) != 1:
                raise ValueError("Expected exactly one Bazel archive output for " + label)
            uri = urlsplit(files[0].get("uri", ""))
            path = Path(unquote(uri.path))
            if uri.scheme != "file" or uri.netloc or not path.is_absolute():
                raise ValueError("Expected a local Bazel archive URI for " + label)
            if not path.is_file() or not path.stat().st_size:
                raise ValueError("Bazel did not materialize a nonempty local archive for " + label)
            outputs[label] = path.parent if is_layout else path
    missing = labels - outputs.keys()
    if missing:
        raise ValueError("Missing Bazel archive outputs: " + ", ".join(sorted(missing)))
    return outputs


def build(args):
    workspace = Path.cwd()
    if not (len(args.archive) == len(args.label) == len(args.owner_args) == len(set(args.archive))):
        raise ValueError("Expected a unique archive, target label and option string for every batch member")
    rc = workspace / "tools/bazel/common.bazelrc"
    shared = settings_from(args.shared_args, rc)
    archives = {}
    owners = defaultdict(list)
    debug_suffix = "-" + args.debug_image_mark + ".gz"
    for name, label, flags in zip(args.archive, args.label, args.owner_args):
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.+-]*\.gz", name):
            raise ValueError("Invalid container archive name: " + name)
        if not label.startswith(("//", "@")):
            raise ValueError("Expected an absolute Bazel target label: " + label)
        for key, value in settings_from([flags], rc).items():
            if shared.get(key) != value:
                raise ValueError(f"{name}: owner option --{key}={value} is not certified for the shared build")
        archives[name] = label
        owner = name.removesuffix(debug_suffix) + ".gz" if name.endswith(debug_suffix) else name
        owners[owner].append(name)
    labels = build_labels(workspace, archives)
    validation_labels = []
    if args.validation_directory:
        from tools.bazel.ci import vs_validation_bundles as validation
        families = validation.validation_targets(archives, set(args.registered_debug), args.validation_debug == "y",
                                                 args.validation_only_debug)
        # Top-level requests can select each owner's private test fixtures.
        validation_labels = validation.extra_targets(families)
    settings = os.environ
    layout_labels = {}
    if args.boot_layouts:
        native = build_helpers.boot_layout_labels(settings.get("BAZEL", "bazel"), archives.values())
        layout_labels = {name: label + ".boot-oci" for name, label in archives.items()
                         if label + ".boot-oci" in native}
    cache_directory = settings.get("BAZEL_CONTAINER_CACHE_DIR", settings.get("BAZEL_SWSS_CACHE_DIR"))
    global_args = settings.get("BAZEL_CONTAINER_ARGS", settings.get("BAZEL_SWSS_ARGS", ""))
    options = build_helpers.cache_options(cache_directory) + shlex.split(global_args)
    options.extend(f"--{key}={value}" for key, value in sorted(shared.items()))
    options.extend(build_helpers.remote_cache_options())
    platform = {"amd64": "x86_64_trixie", "arm64": "aarch64_trixie"}[args.architecture]
    platform = "@sonic_build_infra//platforms:" + platform
    options.extend(["--platforms=" + platform, "--host_platform=" + platform])
    directory = settings.get("SONIC_VS_CONTAINER_BATCH_TIMING_DIR")
    build_timing = Path(directory) if directory else None
    export_timings = {}
    for owner, names in owners.items():
        timing = None
        if build_timing:
            timing = time_vs_image.start_recipe(workspace, owner, [], [], ["export-" + n for n in names])
        export_timings.update(dict.fromkeys(names, timing))
    if build_timing:
        # Preserve original producers and this invocation's exact membership.
        targets = {name: [owner, archives[name]] for owner, names in owners.items() for name in names}
        (build_timing / "batch-targets.json").write_text(json.dumps(targets) + "\n")
    with tempfile.TemporaryDirectory(prefix="sonic-container-batch-") as events_dir:
        events = Path(events_dir) / "events.json"
        command = [
            settings.get("BAZEL", "bazel"), EXPORT_POLICY, "build", *options,
            "--build_event_json_file=" + str(events),
            "--nobuild_event_json_file_path_conversion",
            "--experimental_build_event_output_group_mode=default=inline_only",
            *dict.fromkeys([*labels.values(), *layout_labels.values(), *validation_labels]),
        ]
        try:
            time_vs_image.run_phase(build_timing, "build", command)
        except subprocess.CalledProcessError as failure:
            if (settings.get("SONIC_VS_RETRY_GO_FETCH") != "1" or failure.returncode != 1
                    or not go_proxy_timed_out(events)):
                raise
            print("The Go module proxy TLS handshake timed out. Retrying the container batch once.",
                  file=sys.stderr, flush=True)
            events.unlink()
            if build_timing:
                retry = time_vs_image.start_recipe(workspace, "vs-container-images", ["build"], ["build"], [])
                shutil.copyfile(build_timing / "batch-targets.json", retry / "batch-targets.json")
                build_timing = retry
            time_vs_image.run_phase(build_timing, "build", command)
        outputs = archive_outputs(events, set(labels.values()) | set(layout_labels.values()), set(layout_labels.values()))
        for name, label in labels.items():
            with time_vs_image.time_phase(export_timings[name], "export-" + name):
                destination = args.target_path / name
                build_helpers.export_archive(outputs[label], destination)
                if name in layout_labels:
                    from tools.bazel.oci.boot_layout import publish_layout
                    publish_layout(outputs[layout_labels[name]], outputs[label], destination)
        if validation_labels:
            paths = validation.configured_outputs(settings.get("BAZEL", "bazel"), options, validation_labels, events)
            validation.write_bundles(args, families, paths, shared["//tools/bazel/oci:image_version"],
                                     dialout=shared.get(validation.GNMI_DIALOUT))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-path", type=Path, required=True)
    parser.add_argument("--boot-layouts", action="store_true")
    parser.add_argument("--architecture", choices=("amd64", "arm64"), required=True)
    parser.add_argument("--debug-image-mark", default="dbg")
    parser.add_argument("--validation-directory", type=Path, help="Publish per-image runtime-test artifacts")
    parser.add_argument("--validation-debug", choices=("y", "n"), default="n")
    parser.add_argument("--registered-debug", action="append", default=[])
    parser.add_argument("--validation-only-debug", action="append", default=[])
    parser.add_argument("--shared-args", action="append", default=[],
                        help="Owner-declared Starlark build settings safe for the entire batch")
    for option in ("--archive", "--label", "--owner-args"):
        parser.add_argument(option, action="append", required=True)
    args = parser.parse_args()
    try:
        build(args)
    except subprocess.CalledProcessError as error:
        return time_vs_image.exit_status(error.returncode)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
