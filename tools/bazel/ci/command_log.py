"""Retain command output and timings separately from queried artifact paths."""

import subprocess
import time


def execute(command, directory, receipt, name, *, cwd, output_path=None):
    """Keep diagnostics separate from query results and optional private output.

    Large or sensitive query results can go directly to an already-created
    file. That stdout is neither logged nor printed; the caller owns its mode
    and lifetime. Ordinary queries still return only stdout for path lookup.
    """
    started = time.monotonic()
    if output_path is None:
        result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    else:
        with output_path.open("w") as output:
            result = subprocess.run(command, cwd=cwd, text=True,
                                    stdout=output, stderr=subprocess.PIPE)
    stdout = result.stdout or ""
    (directory / (name + ".log")).write_text(stdout + result.stderr)
    print(stdout, end="", flush=True)
    print(result.stderr, end="", flush=True)
    receipt["commands"].append({"argv": command, "returncode": result.returncode,
                                "elapsed_seconds": time.monotonic() - started,
                                "log": name + ".log"})
    result.check_returncode()
    return stdout
