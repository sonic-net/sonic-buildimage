"""Retain command output and timings separately from queried artifact paths."""

import re
import subprocess
import time


def transient_fetch_failure(stderr):
    """Retry explicit repository transport failures, never unrelated diagnostics."""
    transient = re.compile(
        r"(?:fetch_repo:|Error downloading).*https?://.*"
        r"(?:\bEOF\b|connection reset by peer|connection refused|"
        r"i/o timeout|connect(?:ion)? timed out|TLS handshake timeout|"
        r"temporary failure in name resolution|"
        r"\b(?:429 Too Many Requests|50[0234] (?:Internal Server Error|Bad Gateway|"
        r"Service Unavailable|Gateway Timeout))\b)", re.I)
    if not transient.search(stderr) or re.search(
            r"checksum|SECURITY ERROR|certificate|invalid version|unknown revision", stderr, re.I):
        return False
    for line in stderr.splitlines():
        if re.search(r"fetch_repo:|Error downloading", line, re.I) and not transient.search(line):
            return False
        if not re.search(r"\b(?:ERROR|Error in fail|error|fatal error|FAILED):", line):
            continue
        if transient.search(line):
            continue
        if "An error occurred during the fetch of repository" in line:
            continue
        if "Analysis of target" in line and "failed" in line:
            continue
        if line.strip() == "ERROR: Build did NOT complete successfully":
            continue
        return False
    return True


def execute(command, directory, receipt, name, *, cwd, output_path=None, retry_fetch=False):
    """Keep diagnostics separate from query results and optional private output.

    Large or sensitive query results can go directly to an already-created
    file. That stdout is neither logged nor printed; the caller owns its mode
    and lifetime. Ordinary queries still return only stdout for path lookup.
    Bazel callers can opt into three attempts for explicit transient repository
    fetch failures; attempt logs and total wall time remain in the receipt.
    """
    started = time.monotonic()
    attempts = []
    delays = (10, 30)
    for attempt in range(len(delays) + 1):
        attempt_started = time.monotonic()
        if output_path is None:
            result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
        else:
            with output_path.open("w") as output:
                result = subprocess.run(command, cwd=cwd, text=True,
                                        stdout=output, stderr=subprocess.PIPE)
        stdout = result.stdout or ""
        retry = (retry_fetch and result.returncode == 1 and attempt < len(delays)
                 and transient_fetch_failure(result.stderr))
        log = name + (f".attempt-{attempt + 1}" if retry else "") + ".log"
        (directory / log).write_text(stdout + result.stderr)
        print(stdout, end="", flush=True)
        print(result.stderr, end="", flush=True)
        evidence = {"returncode": result.returncode,
                    "elapsed_seconds": time.monotonic() - attempt_started, "log": log}
        attempts.append(evidence)
        if not retry:
            break
        evidence["retry_delay_seconds"] = delays[attempt]
        print(f"Transient Bazel repository fetch failure; retrying attempt "
              f"{attempt + 2}/3 in {delays[attempt]}s.", flush=True)
        time.sleep(delays[attempt])
    evidence = {"argv": command, "returncode": result.returncode,
                "elapsed_seconds": time.monotonic() - started, "log": name + ".log"}
    if len(attempts) > 1:
        evidence["attempts"] = attempts
    receipt["commands"].append(evidence)
    result.check_returncode()
    return stdout
