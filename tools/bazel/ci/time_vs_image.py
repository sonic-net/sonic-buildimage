"""Record wall time and Bazel profiles for the VS installer's Make recipes."""

import argparse
from contextlib import contextmanager
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import public_artifacts


def start_recipe(workspace, owner, bazel, profiles, exports):
    root = workspace / "artifacts/vs/image-timings" / owner
    root.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(dir=root, prefix="recipe-"))
    (directory / "recipe.json").write_text(json.dumps({
        "bazel": bazel, "profiles": profiles, "exports": exports,
    }) + "\n")
    return directory


def exit_status(returncode):
    return returncode if returncode >= 0 else 128 - returncode


def preserve_bep(source, destination):
    try:
        public_artifacts.write_public_bep(source, destination)
    except (OSError, ValueError, TypeError, AttributeError, RecursionError):
        print("Public BEP capture unavailable: missing or invalid event stream.", file=sys.stderr)


@contextmanager
def capture_bep(directory, phase, command, *, cwd):
    if directory is None or phase not in ("build", "test"):
        yield command
        return
    with tempfile.TemporaryDirectory(prefix="sonic-bep-") as temporary:
        events = None
        for index, argument in enumerate(command):
            if argument.startswith("--build_event_json_file="):
                events = argument.split("=", 1)[1]
            elif argument == "--build_event_json_file" and index + 1 < len(command):
                events = command[index + 1]
        if events:
            source = Path(cwd or Path.cwd()) / events
        else:
            source = Path(temporary) / "events.json"
            index = command.index("--") if "--" in command else len(command)
            command = [*command[:index], "--build_event_json_file=" + str(source), *command[index:]]
        try:
            yield command
        finally:
            preserve_bep(source, directory / f"{phase}.bep.json")


@contextmanager
def log_make_phase(name, *, vs_make):
    """Write a paired VS build-log marker only when the phase succeeds."""
    enabled = vs_make and os.environ.get("SONIC_VS_IMAGE_TIMINGS") == "1"
    if enabled:
        print(f"[ vs timing ] [ {name} ] [ start ]", file=sys.stderr, flush=True)
    yield
    if enabled:
        print(f"[ vs timing ] [ {name} ] [ end ]", file=sys.stderr, flush=True)


@contextmanager
def time_phase(directory, phase):
    if directory is None:
        yield
        return
    start_epoch = time.time()
    start = time.monotonic()
    code = 1
    try:
        yield
        code = 0
    except subprocess.CalledProcessError as error:
        code = exit_status(error.returncode)
        raise
    except FileNotFoundError:
        code = 127
        raise
    finally:
        record = {"phase": phase, "start_epoch_seconds": start_epoch,
                  "elapsed_seconds": time.monotonic() - start, "exit_code": code}
        (directory / f"{phase}.timing.json").write_text(json.dumps(record) + "\n")


def run_phase(directory, phase, command, *, cwd=None, env=None):
    if directory is not None and (phase in ("build", "test") or phase.startswith("cquery-")):
        # Nested workspaces may pass startup flags before the Bazel command.
        index = command.index(phase.split("-", 1)[0], 1) + 1
        profile = directory.resolve() / f"{phase}.profile.json.gz"
        flags = [f"--profile={profile}", "--noslim_profile"]
        if directory.parent.name == "vs-container-images":
            flags.append("--experimental_profile_include_target_label")
        command = [*command[:index], *flags, *command[index:]]
    with capture_bep(directory, phase, command, cwd=cwd) as command, time_phase(directory, phase):
        return subprocess.run(command, check=True, cwd=cwd, env=env)


def main(arguments):
    if arguments[0] == "start-recipe":
        parser = argparse.ArgumentParser()
        parser.add_argument("workspace", type=Path)
        parser.add_argument("owner")
        parser.add_argument("--bazel", nargs="+", required=True)
        parser.add_argument("--profiles", nargs="+", required=True)
        parser.add_argument("--exports", nargs="*", required=True)
        args = parser.parse_args(arguments[1:])
        print(start_recipe(args.workspace, args.owner, args.bazel, args.profiles, args.exports))
        return 0
    if arguments[0] == "image-recipe":
        images = arguments[1:]
        commands = ["build", *("cquery-" + image for image in images)]
        directory = start_recipe(Path.cwd(), images[0], commands, commands,
                                 ["export-" + image for image in images])
        print(directory)
        return 0
    directory, phase, *command = arguments
    try:
        run_phase(Path(directory), phase, command)
    except subprocess.CalledProcessError as error:
        return exit_status(error.returncode)
    except FileNotFoundError as error:
        print(error, file=sys.stderr)
        return 127
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
