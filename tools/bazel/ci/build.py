"""Build and export declared wheel archives and container CI artifact groups."""

from functools import partial
from pathlib import Path

from tools.bazel.build_helpers import export_archive
from tools.bazel.ci import artifact_validation, command_log


def collect_archives(workspace, directory, receipt, targets, *, bazel="bazel", options=()):
    """Build filename-to-label declarations, exporting one nonempty file each.

    The caller owns target selection and payload validation. These are archive
    artifacts: publication uses the shared helper's mode 0644 and keeps a prior
    output intact if a query or copy fails. Commands and completed exports remain
    in the caller's receipt on failure.
    """
    if not targets or any(not name or Path(name).name != name or name in (".", "..") for name in targets):
        raise ValueError("archive declarations need nonempty, plain output filenames")
    execute = partial(command_log.execute, cwd=workspace)
    receipt.setdefault("artifacts", {})
    execute([bazel, "build", *options,
             "--build_event_json_file=" + str(directory / "build-events.jsonl"),
             *targets.values()], directory, receipt, "build")
    paths = {}
    for name, target in targets.items():
        files = execute([bazel, "cquery", *options, "--output=files", target],
                        directory, receipt, name + ".query").splitlines()
        if len(files) != 1:
            raise ValueError("Expected exactly one nonempty archive output for " + target)
        source = workspace / files[0]
        if not source.is_file() or not source.stat().st_size:
            raise ValueError("Expected exactly one nonempty archive output for " + target)
        destination = directory / name
        export_archive(source, destination)
        paths[name] = destination
        receipt["artifacts"][name] = {"target": target, "bytes": destination.stat().st_size,
                                      "sha256": artifact_validation.sha(destination)}
    return paths


def collect_artifacts(workspace, directory, receipt, target, *, bazel="bazel", options=(), execution_log=None):
    """Build an owner filegroup and export its declared files under unique basenames."""
    execute = partial(command_log.execute, cwd=workspace)
    log_options = ["--execution_log_json_file=" + str(execution_log)] if execution_log is not None else []
    execute([bazel, "build", *options, *log_options,
             "--build_event_json_file=" + str(directory / "build-events.jsonl"), target],
            directory, receipt, "build")
    files = execute([bazel, "cquery", *options, "--output=files", target],
                    directory, receipt, "artifacts.query").splitlines()
    names = [Path(name).name for name in files]
    if len(names) != len(set(names)):
        raise ValueError("CI artifacts must have unique output filenames")
    sources = [workspace / name for name in files]
    for source in sources:
        if not source.is_file() or not source.stat().st_size:
            raise ValueError("Expected a nonempty CI artifact file: " + str(source))
    # Some evidence is written after artifact collection. Reserve those names
    # as well as files/logs already produced by the runner.
    reserved = {"receipt.json", "report.json", "MODULE.bazel.lock", "module-graph.json",
                "module-graph.stderr", "module-graph-inspection.json", "test-events.jsonl",
                "build-events.jsonl", "execution-gate-audit.json"}
    for name in names:
        if name in reserved or (directory / name).exists():
            raise ValueError("CI artifact filename collides with build evidence: " + name)
    paths = {}
    receipt["artifacts"] = {}
    for name, source, output in zip(names, sources, files):
        destination = directory / name
        export_archive(source, destination)
        paths[name] = destination
        receipt["artifacts"][name] = {"target": target, "output": output,
                                      "bytes": destination.stat().st_size,
                                      "sha256": artifact_validation.sha(destination)}
    return paths
